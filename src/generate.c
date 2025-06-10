#include "../inc/generate.h"
#include "../inc/logging.h"
#include "../inc/board.h"
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
typedef struct {
	static_arr_info n; 
	dynamic_arr_info nret;
	dynamic_arr_info n2; 
	dynamic_arr_info n4;
	size_t start; 
	size_t end; 
	char nox;
} arguments;
void write_boards(const static_arr_info n, const char* fmt, const int layer){
	static clock_t old;
	static bool startup_init = 0;
	size_t filename_size = strlen(fmt) + 10; // if there are more than 10 digits of layers i'll eat my shoe
	char* filename = malloc_errcheck(sizeof(char) * filename_size);
	snprintf(filename, filename_size, fmt, layer);
	LOGIF(LOG_INFO_){
		printf("[INFO] Writing %lu boards to %s (%lu bytes)\n", n.size, filename, sizeof(uint64_t) * n.size);  // lol
	}	
	if(!startup_init){
		startup_init = true;
		old = clock();
	}
	else{
		clock_t curr = clock();
		size_t diff = curr - old;
		old = curr;
		LOGIF(LOG_INFO_){
			printf("[INFO] Speed: %ld thousand boards per second\n", (long int)(n.size / (1000.0f * diff / CLOCKS_PER_SEC)));
		}
	}
	FILE *file = fopen(filename, "wb");
	if(file == NULL){
		char *buf = malloc_errcheck(100);
		snprintf(buf, 100, "Couldn't write to %s!\n", filename);
		log_out(buf, LOG_WARN_);
		free(buf);
		free(filename);
		return;
	}
	fwrite(n.bp, n.size, sizeof(uint64_t), file);
	fclose(file);
	free(filename);
}

bool checkx(uint64_t board, char x){
	for(int i = 0; i < 16; i++){
		if((GET_TILE(board, i)) == x)
			return false;
	}
	LOGIF(LOG_TRACE_){
		printf("Board %016lx does not contain %d\n", board, x);
	}
	return true;
}

void generation_thread_move(void* data){
	arguments *args = data;
	uint64_t tmp;
	uint64_t old;
	for(size_t i = args->start; i < args->end; i++){
		old = args->n.bp[i];
		tmp = old;
		for(dir d = left; d <= down; d++){
			if(movedir(&tmp, d)){
				canonicalize_b(&tmp); // TODO it's not necessary to gen *all* boards in nox
				push_back(&args->nret, tmp);
				tmp = old;
			}
		}
	}
}
void generation_thread_spawn(void* data){
	arguments *args = data;
	uint64_t tmp;
	uint64_t old;
	for(size_t i = args->start; i < args->end; i++){
		old = args->n.bp[i];
		for(int tile = 0; tile < 16; tile++){
			if(GET_TILE(old, tile) == 0){
				tmp = old;
				SET_TILE(tmp, tile, 1);
				canonicalize_b(&tmp);
				push_back(&args->n2, tmp);
				tmp = old;
				SET_TILE(tmp, tile, 2);
				canonicalize_b(&tmp);
				push_back(&args->n4, tmp);
			}
		}
	}
}

static void init_threads(const dynamic_arr_info *n, const unsigned int core_count, bool move, threadpool pool, arguments *cores, char nox){
	void (*fn)(void*) = move ? generation_thread_move : generation_thread_spawn;
	for(unsigned i = 0; i < core_count; i++){ // initialize worker threads
		cores[i].n = (static_arr_info){.valid = n->valid, .bp = n->bp, .size = n->sp - n->bp};
		if(move)
			cores[i].nret = init_darr(0, 4 * (n->sp - n->bp));
		else{
			cores[i].n2 = init_darr(0, 4 * (n->sp - n->bp));
			cores[i].n4 = init_darr(0, 4 * (n->sp - n->bp));
			cores[i].nox = nox;
		}
		// divide up [0,n.size)
		// cores work in [start,end)
		int block_size = (n->sp - n->bp) / core_count;
		cores[i].start = i * block_size;
		cores[i].end = (i + 1) * block_size;
		// core_count * n.size / core_count = n.size
		// make sure that the last thread covers all of the array
		if(i + 1 == core_count){
			cores[i].end = n->sp - n->bp;
		}
		thpool_add_work(pool, fn, (void*)(cores + i));
	}
}

void generate_layer(dynamic_arr_info* n, dynamic_arr_info* n2, dynamic_arr_info* n4, 
		const unsigned core_count, const char *fmt_dir, const int layer, threadpool pool, char nox){
	arguments *cores = malloc_errcheck(sizeof(arguments) * core_count);
	// move
	init_threads(n, core_count, true, pool, cores, nox);
	// wait for moves to be done
	thpool_wait(pool);
	destroy_darr(n); // this array currently holds boards where we just spawned -- these are never our responsibility
	*n = init_darr(0,0);
	for(size_t i = 0; i < core_count; i++){
		*n = concat(n, &cores[i].nret);
	}
	deduplicate(n, core_count, pool);
	// spawn
	init_threads(n, core_count, false, pool, cores, nox);
	// write while waiting for spawns
	write_boards((static_arr_info){.valid = n->valid, .bp = n->bp, .size = n->sp - n->bp}, fmt_dir, layer);
	thpool_wait(pool);
	// concatenate spawns
	for(size_t i = 0; i < core_count; i++){
		*n2 = concat(n2, &cores[i].n2);
		*n4 = concat(n4, &cores[i].n4);
	}
	deduplicate_args *args = malloc_errcheck(sizeof(deduplicate_args));
	args->d = n2;
	thpool_add_work(pool, deduplicate_wt, args);
	deduplicate(n4, core_count, pool);
	thpool_wait(pool);
	free(cores);
}
void generate(const int start, const int end, const char* fmt, uint64_t* initial, const size_t initial_len, 
		const unsigned core_count, bool prespawn, char nox, bool free_formation){
	// GENERATE: write all sub-boards where it is the computer's move	
	generate_lut(free_formation);
	threadpool pool = thpool_init(core_count);
	static const size_t DARR_INITIAL_SIZE = 100;
	dynamic_arr_info n  = init_darr(false, 0);
	free(n.bp);
	n.bp = initial;
	n.size = initial_len;
	n.sp = n.size + n.bp;
	dynamic_arr_info n2 = init_darr(false, DARR_INITIAL_SIZE);
	dynamic_arr_info n4 = init_darr(false, DARR_INITIAL_SIZE);
	for(int i = start; i <= end; i += 2){
		generate_layer(&n, &n2, &n4, core_count, fmt, i, pool, nox);
		destroy_darr(&n);
		n = n2;
		n2 = n4;
		n4 = init_darr(false, DARR_INITIAL_SIZE);
	}
	destroy_darr(&n);
	destroy_darr(&n2);
	destroy_darr(&n4);
	thpool_destroy(pool);
}
static_arr_info read_boards(const char *dir){
	FILE *fp = fopen(dir, "rb");
	if(fp == NULL){
		char *buf = malloc_errcheck(100);
		snprintf(buf, 100, "Couldn't read %s!\n", dir);
		log_out(buf, LOG_WARN_);
		free(buf);
		return (static_arr_info){.valid = false};
	}
	fseek(fp, 0L, SEEK_END);
	size_t sz = ftell(fp);
	rewind(fp);
	if(sz % 8 != 0)
		log_out("sz %%8 != 0, this is probably not a real table!\n", LOG_WARN_);
	uint64_t* data = malloc_errcheck(sz);
	fread(data, 1, sz, fp);
	char *buf = malloc_errcheck(100);
	snprintf(buf, 100, "Read %ld bytes (%ld boards) from %s\n", sz, sz / 8, dir);
	log_out(buf, LOG_DBG_);
	free(buf);
	fclose(fp);
	static_arr_info res = {true, data, sz / 8}; 
#ifdef DBG
	uint64_t tmp;
	for(size_t i = 0; i < sz / 8; i++){
		tmp = res.bp[i];
		canonicalize_b(&tmp);
		if(tmp != res.bp[i]){
			log_out("Reading non canonicalized board!!!!", LOG_WARN_);
		}
	}
	
#endif
	return res;
}
