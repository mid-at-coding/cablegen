#include "../inc/generate.h"
#include "../inc/logging.h"
#include "../inc/main.h"
#include "../inc/board.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
int init_errorcheck_mutex(pthread_mutex_t *mutex)
{
    pthread_mutexattr_t attr;
    int r;

    r = pthread_mutexattr_init(&attr);
    if (r != 0)
        return r;

    r = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    if (r == 0)
        r = pthread_mutex_init(mutex, &attr);

    pthread_mutexattr_destroy(&attr);

    return r;
}
struct {
	static_arr_info n; 
	dynamic_arr_info* nret;
	dynamic_arr_info* n2; 
	dynamic_arr_info* n4;
	pthread_mutex_t* done; 
	size_t start; 
	size_t end; 
	dynamic_arr_info *potential_duplicate;
} typedef arguments;

struct {
	pthread_t core;
	arguments args;
}typedef core_data;

static void write_boards(const static_arr_info n, const char* fmt, const int layer){
	size_t filename_size = strlen(fmt) + 10; // if there are more than 10 digits of layers i'll eat my shoe
	char* filename = malloc_errcheck(sizeof(char) * filename_size);
	snprintf(filename, filename_size, fmt, layer);
	printf("[INFO] Writing %lu boards to %s (%lu bytes)\n", n.size, filename, sizeof(uint64_t) * n.size);  // lol
	FILE *file = fopen(filename, "wb");
	if(file == NULL){
		char *buf = malloc_errcheck(100);
		snprintf(buf, 100, "Couldn't write to %s!\n", filename);
		log_out(buf, LOG_WARN_);
		free(buf);
		return;
	}
	fwrite(n.bp, n.size, sizeof(uint64_t), file);
	fclose(file);
}

bool shifted(uint64_t a, uint64_t b){
    for(int i = 0; i < 16; i++){
    	if(GET_TILE(a, i) == 0xF && GET_TILE(b, i) != 0xF){
		    return true;      		
    	}
    	if(GET_TILE(b, i) == 0xF && GET_TILE(a, i) != 0xF){
		    return true;      		
    	}
    }
	return false;
}

void* generation_thread_move(void* data){
	arguments *args = data;
	uint64_t tmp, tmp2;
	bool (*move)(uint64_t*, dir) = flat_movement ? flat_move : movedir;
	int res = pthread_mutex_trylock(args->done);
	if (res != 0){ // the caller has been naughty
		char *buf = malloc_errcheck(100);
		snprintf(buf, 100, "Worker thread mutex (%lu) couldn't be locked, check caller logic!(%d)\n", (unsigned long)args->done, res);
		log_out(buf, LOG_ERROR_);
		free(buf);
	}
	for(size_t i = args->start; i < args->end; i++){
		tmp = args->n.bp[i];
		tmp2 = args->n.bp[i];
		for(dir d = left; d < down; d++){
			if(move(&tmp, d)){
				if(shifted(tmp2, tmp))
				    continue;
				canonicalize(&tmp);
				push_back(args->nret, tmp);
			}
		}
	}
	pthread_mutex_unlock(args->done);
	return NULL;
}
void* generation_thread_spawn(void* data){
	arguments *args = data;
	int res = pthread_mutex_trylock(args->done);
	uint64_t tmp;
	if (res != 0){ // the caller has been naughty
		char *buf = malloc_errcheck(100);
		snprintf(buf, 100, "Worker thread mutex (%lu) couldn't be locked, check caller logic!(%d)\n", (unsigned long)args->done, res);
		log_out(buf, LOG_ERROR_);
		free(buf);
	}
	for(size_t i = args->start; i < args->end; i++){
		for(int tile = 0; tile < 16; tile++){
			if(GET_TILE((args->n).bp[i], tile) == 0){
				tmp = args->n.bp[i];
				SET_TILE(tmp, tile, 1);
				canonicalize(&tmp);
				push_back(args->n2, tmp);
				tmp = args->n.bp[i];
				SET_TILE(tmp, tile, 2);
				canonicalize(&tmp);
				push_back(args->n4, tmp);
			}
		}
	}
	pthread_mutex_unlock(args->done);
	return NULL;
}

void init_threads(core_data* cores, dynamic_arr_info n, dynamic_arr_info *potential_duplicate,
		size_t core_count, size_t arr_size_per_thread, bool spawn, void* (worker_thread)(void*)){
	for(uint i = 0; i < core_count; i++){ // initialize worker threads
		if(spawn){
			*cores[i].args.n2 = init_darr(false, arr_size_per_thread / 2);
			*cores[i].args.n4 = init_darr(false, arr_size_per_thread / 2);
		}
		else{
			*cores[i].args.nret = init_darr(false, arr_size_per_thread);
		}

		cores[i].args.n = (static_arr_info){n.valid, n.bp, n.sp - n.bp};
		cores[i].args.potential_duplicate = potential_duplicate;
		// divide up [0,n.size)
		// cores work in [start,end)
		int block_size = n.size / core_count;
		cores[i].args.start = i * block_size;
		cores[i].args.end = (i + 1) * block_size;
		// core_count * n.size / core_count = n.size
		// make sure that the last thread covers all of the array
		if(i + 1 == core_count){
			cores[i].args.end = n.sp - n.bp;
		}
		int res = pthread_create(&cores[i].core, NULL, worker_thread, (void*)(&cores[i].args));
		if(res != 0){
			char *buf = malloc_errcheck(100);
			snprintf(buf, 100, "Core initialization failed: Error %d, Core %d", res, i);
			log_out(buf, LOG_ERROR_);
			free(buf);
		}
	}
}

void wait_all(core_data *cores, size_t core_count){
	static_arr_info done_arr = init_sarr(false, core_count);
	bool done = false;
	while(done == false){
		done = true;
		for(uint i = 0; i < core_count; i++){
			done_arr.bp[i] = pthread_mutex_trylock(cores[i].args.done);
			if (done_arr.bp[i] != 0)
				done = false;
			else
				pthread_mutex_unlock(cores[i].args.done); // we own this mutex so we have to unlock it before we try to lock it again
		}
	}
	// if we're here, all the cores are done
	for(uint i = 0; i < core_count; i++){
		pthread_join(cores[i].core, NULL); // idc abt the return
	}
}

void generate_layer(dynamic_arr_info * restrict n, dynamic_arr_info * restrict n2, dynamic_arr_info * restrict n4, 
		dynamic_arr_info * restrict potential_duplicate, const uint core_count, const char *fmt_dir, const int layer){ // this is completely nonsensical if any of the pointers are the same, hence restrict
	// make an initial estimate for the amount of spawns per tile: TODO test efficacy of different values
	size_t approx_len = 24 * n->size; // 6 open spaces and 4 directions: definitely an overestimate
	size_t arr_size_per_thread = approx_len / core_count;
	static bool core_init = false;
	static core_data* cores;

	if(!core_init){
		cores = malloc_errcheck(sizeof(core_data) * core_count); // make an array of core data
		for(size_t i = 0; i < core_count; i++){
			cores[i].args.done = malloc_errcheck(sizeof(pthread_mutex_t));
			cores[i].args.nret = malloc_errcheck(sizeof(dynamic_arr_info));
			cores[i].args.n2 = malloc_errcheck(sizeof(dynamic_arr_info));
			cores[i].args.n4 = malloc_errcheck(sizeof(dynamic_arr_info));
			cores[i].args.potential_duplicate = malloc_errcheck(sizeof(dynamic_arr_info));
			int res = init_errorcheck_mutex(cores[i].args.done);
			if(res != 0){
				printf("Failed to init core mutex: errcode %d", res);
				exit(res);
			}
		}
		core_init = true;
	}
	init_threads(cores, *n, potential_duplicate, core_count, arr_size_per_thread, false, generation_thread_move);
	// twiddle our thumbs
	// TODO: there's some work we can do by concating all the results as they're coming in instead of waiting for all of them
	wait_all(cores, core_count);
	// concatenate all the data TODO: maybe there is some clever way to do this?
	for(uint i = 0; i < core_count; i++){ 
		*n = concat(n, cores[i].args.nret);
	}
	deduplicate(n);

	init_threads(cores, *n, potential_duplicate, core_count, arr_size_per_thread, true, generation_thread_spawn);
	// write while we're waiting for the spawning threads
	write_boards((static_arr_info){n->valid, n->bp, n->sp - n->bp}, fmt_dir, layer);
	wait_all(cores, core_count);
	for(uint i = 0; i < core_count; i++){ 
		*n2 = concat(n2, cores[i].args.n2);
		*n4 = concat(n4, cores[i].args.n4);
	}
	deduplicate(n);
	deduplicate(n4);
	deduplicate(n2);
}
void generate(const int start, const int end, const char* fmt, uint64_t* initial, const size_t initial_len, const uint core_count, bool prespawn){
	dynamic_arr_info n, n2, n4, potential_duplicate;
	n.bp = initial;
	n.sp = n.bp + initial_len;
	n.size = initial_len;
	// initialize arrays
	n2 = init_darr(false, 0);
	n4 = init_darr(false, 0);
	potential_duplicate = init_darr(false, 0);
	if(prespawn){ // spawn once before moving
		log_out("Prespawning...\n", LOG_DBG_);
		for(uint64_t *curr = n.bp; curr < n.sp; curr++){
			for(int i = 0; i < 16; i++){
				if(GET_TILE((*curr), i) == 0){
					SET_TILE((*curr), i, 1);
					push_back(&n2, (*curr));
					SET_TILE((*curr), i, 2);
					push_back(&n4, (*curr));
					SET_TILE((*curr), i, 0);
				}
			}
		}
	}
	log_out("Generating...\n", LOG_DBG_);
	log_out("Starting boards: \n", LOG_DBG_);
	for(int i = 0; i < initial_len; i++)
		printf("0x%016lx\n",n.bp[i]);
	for(int i = 0; i < n2.sp - n2.bp; i++)
		printf("0x%016lx\n",n2.bp[i]);
	for(int i = 0; i < n4.sp - n4.bp; i++)
		printf("0x%016lx\n",n4.bp[i]);
	for(int i = start; i < end; i += 2){
		generate_layer(&n, &n2, &n4, &potential_duplicate, core_count, fmt, i);
		destroy_darr(&n);
		n = n2;
		n2 = n4;
		n4 = init_darr(false, 0);
	}
}

static_arr_info read_table(const char *dir){
	FILE *fp = fopen(dir, "rb");
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
	return (static_arr_info){true, data, sz / 8}; 
}

void destroy_darr(dynamic_arr_info* arr){
	free(arr->bp);
	arr->valid = false;
}
void destroy_sarr(static_arr_info* arr){
	free(arr->bp);
	arr->valid = false;
}
