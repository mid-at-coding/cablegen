#include "../inc/generate.h"
#include "../inc/logging.h"
#include "../inc/board.h"
#include "../inc/main.h"
#include "../inc/thpool.h"
#include "../inc/settings.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
struct {
	static_arr_info n; 
	dynamic_arr_info nret;
	dynamic_arr_info n2; 
	dynamic_arr_info n4;
	size_t start; 
	size_t end; 
} typedef arguments;
void write_boards(const static_arr_info n, const char* fmt, const int layer){
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
bool shiftedr(uint64_t a, uint64_t b){
	uint64_t *rots_a = get_all_rots(a);
	for(int i = 0; i < 8; i++){
		if(!shifted(rots_a[i], b)){
			return false;
		}
	}
	return true;
}
void generation_thread_move(void* data){
	arguments *args = data;
	uint64_t tmp, tmp2;
	bool (*move)(uint64_t*, dir) = movedir; //flat_movement ? flat_move : movedir;
	for(size_t i = args->start; i < args->end; i++){
		tmp = args->n.bp[i];
		tmp2 = args->n.bp[i];
		for(dir d = left; d < down; d++){
			if(move(&tmp, d)){
				canonicalize_b(&tmp);
				push_back(&args->nret, tmp);
			}
		}
	}
}
void generation_thread_spawn(void* data){
	arguments *args = data;
	uint64_t tmp;
	for(size_t i = args->start; i < args->end; i++){
		for(int tile = 0; tile < 16; tile++){
			if(GET_TILE((args->n).bp[i], tile) == 0){
				tmp = args->n.bp[i];
				SET_TILE(tmp, tile, 1);
				canonicalize_b(&tmp);
				push_back(&args->n2, tmp);
				tmp = args->n.bp[i];
				SET_TILE(tmp, tile, 2);
				canonicalize_b(&tmp);
				push_back(&args->n4, tmp);
			}
		}
	}
}
void init_thread_data(arguments *cores, const size_t core_count, const size_t spawn_reserve, const size_t move_reserve, const bool spawn, 
		const dynamic_arr_info *n){
	for(uint i = 0; i < core_count; i++){ // initialize worker threads
		if(spawn){
			cores[i].n2 = init_darr(false, spawn_reserve);
			cores[i].n4 = init_darr(false, spawn_reserve);
		}
		else
			cores[i].nret = init_darr(false, move_reserve);

		cores[i].n = (static_arr_info){n->valid, n->bp, n->sp - n->bp};
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
	}
}
void generate_layer(dynamic_arr_info* n, dynamic_arr_info* n2, dynamic_arr_info* n4, 
		const uint core_count, const char *fmt_dir, const int layer, threadpool *pool){
	arguments *args = malloc_errcheck(core_count * sizeof(arguments));
	const size_t move_reserve = n->size * 2; // assume about 2 moves per board in n
	init_thread_data(args, core_count, 0, move_reserve, false, n);
	for(int i = 0; i < core_count; i++){
		thpool_add_work((*pool), generation_thread_move, (void*)(&args[i]));
	}	
	// twiddle our thumbs
	// TODO: there's some work we can do by concating all the results as they're coming in instead of waiting for all of them
	thpool_wait(*pool);
	// concatenate all the data TODO: maybe there is some clever way to do this?
//	destroy_darr(n); // in n right now is boards that came from a spawn -- these boards we don't care about, we only write the results of moves
//	*n = init_darr(0,move_reserve);
	for(uint i = 0; i < core_count; i++){ 
		*n = concat(n, &args[i].nret);
	}
	deduplicate(n);
	const size_t spawn_reserve = n->size * 4; // assume about 4 spawns per board in n
	init_thread_data(args, core_count, spawn_reserve, 0, true, n);
	for(int i = 0; i < core_count; i++){
		thpool_add_work((*pool), generation_thread_spawn, (void*)(&args[i]));
	}
	write_boards((static_arr_info){.valid = n->valid, .bp = n->bp, .size = n->sp - n->bp}, fmt_dir, layer);
	thpool_wait(*pool);
	for(uint i = 0; i < core_count; i++){ 
		*n2 = concat(n2, &args[i].n2);
		*n4 = concat(n4, &args[i].n4);
	}
	deduplicate(n2);
	deduplicate(n4);
#ifdef DBG
	for(uint64_t *b = n->bp; b < n->sp; b++){
		if(get_sum(*b) != layer){
			log_out("Layer mismatch!", LOG_DBG_);
		}
	}
	for(uint64_t *b = n2->bp; b < n2->sp; b++){
		if(get_sum(*b) != layer + 2){
			log_out("Layer mismatch!", LOG_DBG_);
		}
	}
	for(uint64_t *b = n4->bp; b < n4->sp; b++){
		if(get_sum(*b) != layer + 4){
			log_out("Layer mismatch!", LOG_DBG_);
		}
	}
#endif
}
void generate(const int start, const int end, const char* fmt, uint64_t* initial, const size_t initial_len, const uint core_count, bool prespawn){
	bool free_formation = 0; // TODO, should boards that don't come from moving n really stay?
	get_bool_setting("free_formation", &free_formation);
	generate_lut(free_formation);
	threadpool pool = thpool_init(core_count);
	const static size_t DARR_INITIAL_SIZE = 100;
	dynamic_arr_info n  = init_darr(false, 0);
	n.bp = initial;
	n.size = initial_len;
	n.sp = n.size + n.bp;
	dynamic_arr_info n2 = init_darr(false, DARR_INITIAL_SIZE);
	dynamic_arr_info n4 = init_darr(false, DARR_INITIAL_SIZE);
	for(int i = start; i <= end; i += 2){
		generate_layer(&n, &n2, &n4, core_count, fmt, i, &pool);
		destroy_darr(&n);
		n = n2;
		n2 = n4;
		n4 = init_darr(false, DARR_INITIAL_SIZE);
	}
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
