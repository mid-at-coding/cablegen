#include "generate.h"
#include "array.h"
#include <bits/pthreadtypes.h>
#include <stdlib.h>
#ifdef BENCH
#include "bench.h"
#endif
#include <time.h>
#define LOG_H_ENUM_PREFIX_
#define LOG_H_NAMESPACE_ 
#include "logging.h"
#include "format.h"
#include "board.h"
#include "settings.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#define STR(x) #x
#define EXPAND_STR(x) STR(x)
#define VERSION_STR EXPAND_STR(VERSION)

typedef struct {
	static_arr_info n; 
	dynamic_arr_info n2; 
	dynamic_arr_info n4;
	size_t start; 
	size_t end; 
	long stsl; 
	long smallest_large; 
	long ltc;
	char nox;
	long layer;
	pthread_t thread;
} arguments;

void print_speed(size_t size){
	static bool init = false;
	static bool enabled = true;
	static struct timespec old_time;
	struct timespec time;
	if(!enabled)
		return;
	if(clock_gettime(CLOCK_MONOTONIC, &time)){
		log_out("Could not get system time!", LOG_WARN);
		return;
	}
	if(!init){
		init = true;
/*		if(clock_getcpuclockid(0, &clock)){
			log_out("Could not get system time, speed information will be disabled!", LOG_WARN);
			enabled = false;
			return;
		} */
		if(clock_gettime(CLOCK_MONOTONIC, &old_time)){
			log_out("Could not get system time, speed information will be disabled!", LOG_WARN);
			enabled = false;
			return;
		}
		return; // dont need to display on the first layer bc we don't know when startup was
	}
	struct timespec diff = { 
		.tv_sec  = time.tv_sec  - old_time.tv_sec,
		.tv_nsec = time.tv_nsec - old_time.tv_nsec };
	old_time = time;
	long totalns = (1'000'000'000 * diff.tv_sec) + diff.tv_nsec;
	if(totalns == 0){
		return;
	}
	logf_out("Speed: %ld thousand boards per second", LOG_INFO, (long)((float)((float)size / 1000)/ ((float)totalns / (float)1000000000)));
}

void write_boards(const static_arr_info n, const char* fmt, const int layer){
	char* filename = format_str(fmt, layer);
	logf_out("Writing %lu boards to %s (%lu bytes)", LOG_INFO, n.size, filename, sizeof(uint64_t) * n.size);
	FILE *file = fopen(filename, "wb");
	if(file == NULL){
		logf_out("Couldn't write to %s!", LOG_WARN, filename);
		free(filename);
		return;
	}
	fwrite(n.bp, n.size, sizeof(uint64_t), file);
	free(filename);
	fclose(file);
	print_speed(n.size);
}

bool checkx(uint64_t board, char x){
	for(int i = 0; i < 16; i++){
		if((GET_TILE(board, i)) == x)
			return false;
	}
	logf_out("Board %016lx does not contain %d", LOG_TRACE, board, x);
	return true;
}

bool prune_board(const uint64_t board, const long stsl, const long ltc, const long smallest_large){
	return false;
}

void *gen_layer_thread(void *data){
	arguments *args = data;
	dynamic_arr_info ntemp = init_darr(0, args->n.size);
	// move
	uint64_t tmp, old;
	for(size_t i = args->start; i < args->end; i++){
		old = args->n.bp[i];
		tmp = old;
		for(dir d = left; d <= down; d++){
			if(movedir_unstable(&tmp, d)){
				push_back(&ntemp, tmp);
				tmp = old;
			}
		}
	}
	// TODO test if it's ever faster to dedupe here
	// spawn
	for(long long i = 0; i < ntemp.sp - ntemp.bp; i++){ // perhaps more expressive to iterate with the ptr
		old = ntemp.bp[i]; // don't read from n
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
	destroy_darr(&ntemp);
	logf_out("Core done at %ld", LOG_TRACE, time(NULL));
	return NULL;
}

static void init_threads(const dynamic_arr_info *n, const unsigned int core_count, arguments *cores, char nox, long layer){ 
	const size_t INITIAL_SPAWN = 512;
	for(unsigned i = 0; i < core_count; i++){ // initialize worker threads
		cores[i].n = (static_arr_info){.valid = n->valid, .bp = n->bp, .size = n->sp - n->bp};
		cores[i].n2 = init_darr(0, INITIAL_SPAWN);
		cores[i].n4 = init_darr(0, INITIAL_SPAWN);
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
	for(unsigned i = 0; i < core_count; i++){
		int e;
		log_out("Creating thread", LOG_TRACE);
		if((e = pthread_create(&cores[i].thread, NULL, gen_layer_thread, (void*)(cores + i)))){
			log_out("Failed creating thread!", LOG_ERROR);
			exit(EXIT_FAILURE);
		}
	}
}

void wait(arguments *cores, const unsigned core_count){ // TODO: standardize core_count type
	for(size_t i = 0; i < core_count; i++){
		pthread_join(cores[i].thread, NULL);
	}
}

#define BASE 16
#define BITS 4
#define D(v, d) ((v >> (64 - (d * BITS) - BITS)) & 0xf)

typedef struct {
	int digit;
	size_t final;
	uint64_t *arr;
	arguments *args;
	unsigned core_count; 
	bool four_spawn;
} deduplicate_args;

void *deduplicate_worker_thread(void *data){
	deduplicate_args *args = data;
	args->final = 0;
	// a) copy data from main arrays to static arrs
	size_t cursor = 0;
	for(size_t i = 0; i < args->core_count; i++){
		uint64_t *bp, *sp;
		if(args->four_spawn){
			bp = args->args[i].n4.bp;
			sp = args->args[i].n4.sp;
		}
		else{
			bp = args->args[i].n2.bp;
			sp = args->args[i].n2.sp;
		}
		for(uint64_t *curr = bp; curr < sp; curr++){
			if(D((*curr), 0) == args->digit){
				args->arr[cursor++] = *curr;
			}
		}
	}

	// b) sort that data
	if(cursor <= 1){
		logf_out("Made %lu elements with %02lx", LOG_TRACE, cursor, args->digit);
		args->final = cursor;
		return NULL;
	}
	uint64_t *copy = malloc(sizeof(uint64_t) * cursor); // make a copy because it's slow to deduplicate in place
	if(!copy){
		log_f(stderr, "Could not copy array!", LOG_ERROR);
		return NULL;
	}
	memcpy(copy, args->arr, cursor * sizeof(uint64_t));
	qs_sort_h(copy, cursor);

	// c) deduplicate
	size_t cursor_out = 1;
	args->arr[0] = copy[0];
	for(size_t i = 1; i < cursor; i++){
		if(copy[i] > args->arr[cursor_out - 1]){
			args->arr[cursor_out++] = copy[i];
		}
	}
	free(copy);

	// d) return len of final array (saved in args->final)
	logf_out("Made %lu elements with %02lx", LOG_TRACE, cursor_out, args->digit);
	logf_out("Core done at %ld", LOG_TRACE, time(NULL));
	args->final = cursor_out;
	return NULL;
}

static_arr_info combine_spawns(arguments *cores, const unsigned core_count, const bool four_spawn){
	if(four_spawn)
		logf_out("Starting to combine four spawns at %ld", LOG_DBG, time(NULL));
	else
		logf_out("Starting to combine two spawns at %ld", LOG_DBG, time(NULL));
	size_t histogram[BASE] = {};
	// build histogram
	for(size_t core = 0; core < core_count; core++){
		uint64_t *bp, *sp;
		if(four_spawn){
			bp = cores[core].n4.bp;
			sp = cores[core].n4.sp;
		}
		else{
			bp = cores[core].n2.bp;
			sp = cores[core].n2.sp;
		}
		for(uint64_t *curr = bp; curr < sp; curr++){
			histogram[D((*curr), 0)]++;
		}
	}
	
	// make static arrays for each digit
	uint64_t *arrs[BASE];
	for(size_t i = 0; i < BASE; i++){
		arrs[i] = malloc(histogram[i] * sizeof(uint64_t));
		if(!arrs[i]){
			log_f(stderr, "Could not allocate array for deduplication!", LOG_ERROR);
			exit(EXIT_FAILURE);
		}
	}

	for(size_t i = 0; i < BASE; i++){
		logf_out("%02lx: %lu members", LOG_TRACE, i, histogram[i]);
	}

	logf_out("Dispatching %d threads at %ld", LOG_DBG, BASE, time(NULL));

	// create threads to:
	// a) copy data from main arrays to static arrs
	// b) sort that data
	// c) deduplicate
	// d) return the length of the final array
	// ASSUMPTION: core_count <= base (for utilization)
	// ASSUMPTION: the OS will gracefully handle the base where core_count < base 
	// and keep good utilization
	deduplicate_args args[BASE];
	for(size_t i = 0; i < BASE; i++){
		args[i].digit = i;
		args[i].final = 0;
		args[i].arr = arrs[i];
		args[i].args = cores;
		args[i].core_count = core_count;
		args[i].four_spawn = four_spawn;
	}
	pthread_t threads[BASE];
	for(size_t i = 0; i < BASE; i++){
		int e = 0;
		if((e = pthread_create(&threads[i], NULL, deduplicate_worker_thread, args + i))){
			log_out("Failed creating thread!", LOG_ERROR);
			exit(EXIT_FAILURE);
		}
	}

	// wait for threads and compute final len
	log_out("Waiting for cores...", LOG_DBG);
	size_t len = 0;
	for(size_t i = 0; i < BASE; i++){
		pthread_join(threads[i], NULL);
		len += args[i].final;
	}

	// free the data
	if(four_spawn){
		for(size_t i = 0; i < core_count; i++){
			destroy_darr(&cores[i].n4);
		}
	}
	else{
		for(size_t i = 0; i < core_count; i++){
			destroy_darr(&cores[i].n2);
		}
	}

	// make final arr
	uint64_t *final = malloc(sizeof(uint64_t) * len);
	if(!final){
		log_f(stderr, "Could not allocate array for deduplication!", LOG_ERROR);
		exit(EXIT_FAILURE);
	}
	uint64_t *cursor = final;
	for(size_t i = 0; i < BASE; i++){
		memcpy(cursor, args[i].arr, args[i].final * sizeof(uint64_t));
		cursor += args[i].final;
		free(args[i].arr);
	}

	logf_out("Done combining spawns %ld", LOG_DBG, time(NULL));
	return (static_arr_info){.valid = true, .bp = final, .size = len};
}

typedef struct {
	dynamic_arr_info *curr;
	arguments *cores;
	unsigned int core_count;
	bool spawn;
} combine_spawns_async_args;

void *combine_spawns_async(void *data){
	combine_spawns_async_args args = *(combine_spawns_async_args*)data;
	*args.curr = sarrtodarr(combine_spawns(args.cores, args.core_count, args.spawn));
	return NULL;
}

void generate_layer(dynamic_arr_info* n, dynamic_arr_info* n2, dynamic_arr_info* n4, 
		const unsigned core_count, const char *fmt_dir, const int layer, arguments *cores, char nox){
	log_out("Starting generation", LOG_DBG);
	init_threads(n, core_count, cores, nox, layer);
	log_out("Waiting for cores...", LOG_DBG);
	wait(cores, core_count); // now we have in cores n2 and n4 all of the player's moves
	
	// C threading moment
	pthread_t n2_thread;
	pthread_t n4_thread;
	dynamic_arr_info currn2;
	dynamic_arr_info currn4;
	combine_spawns_async_args argsn2 = {
		.curr = &currn2,
		.cores = cores,
		.core_count = core_count,
		.spawn = false
	};
	combine_spawns_async_args argsn4 = {
		.curr = &currn4,
		.cores = cores,
		.core_count = core_count,
		.spawn = true
	};
	pthread_create(&n2_thread, NULL, combine_spawns_async, &argsn2);
	pthread_create(&n4_thread, NULL, combine_spawns_async, &argsn4);
	pthread_join(n2_thread, NULL);
	pthread_join(n4_thread, NULL);
	destroy_darr(n4);
	*n4 = currn4;
	*n2 = concat(n2, &currn2);
}

void generate(const int start, const int end, const char *fmt, const static_arr_info *initial){
	// GENERATE: write all sub-boards where it is the PLAYER's move
	generate_lut();
	static const size_t DARR_INITIAL_SIZE = 512;
	long long core_count = get_settings()->min.cores; // TODO: perhaps better to send these as settings?
							  // there's kind of a situation where some parameters
							  // are passed as args, and some as settings
	long long nox = get_settings()->min.nox;

	dynamic_arr_info n  = init_darr(false, 0); // technically can simply not initialize, but if a mtx
						   // or other things are added, this is more robust
	free(n.bp);
	n.bp = malloc_errcheck(initial->size * sizeof(uint64_t));
	memcpy(n.bp, initial->bp, initial->size * sizeof(uint64_t));
	n.size = initial->size;
	n.sp = n.size + n.bp;

	dynamic_arr_info n2 = init_darr(false, DARR_INITIAL_SIZE);
	dynamic_arr_info n4 = init_darr(false, DARR_INITIAL_SIZE);

	arguments *cores = malloc_errcheck(sizeof(arguments) * core_count); // allocate space for thread args 

	if(get_settings()->premove){
		log_f(stderr, "Premove uninplemented!", LOG_ERROR);
		exit(EXIT_FAILURE);
	}

	if(get_settings()->prune){
		log_f(stderr, "Pruning uninplemented!", LOG_ERROR);
		exit(EXIT_FAILURE);
	}

	for(int i = start; i <= end; i += 2){
		generate_layer(&n, &n2, &n4, core_count, fmt, i, cores, nox);
		destroy_darr(&n);
		n = n2;
		n2 = n4;
		n4 = init_darr(false, DARR_INITIAL_SIZE);
	}

	destroy_darr(&n);
	destroy_darr(&n2);
	destroy_darr(&n4);
	free(cores);
}

static_arr_info read_boards(const char *dir){
	FILE *fp = fopen(dir, "rb");
	if(fp == NULL){
		goto fail;
	}
	fseek(fp, 0L, SEEK_END);
	size_t sz = ftell(fp);
	rewind(fp);
	if(sz % 8 != 0)
		log_out("sz %%8 != 0, this is probably not a real table!", LOG_WARN);
	uint64_t* data = malloc_errcheck(sz);
	if(fread(data, 1, sz, fp) != sz){
		goto fail;
	}
	logf_out("Read %ld bytes (%ld boards) from %s", LOG_INFO, sz, sz / 8, dir);
	fclose(fp);
	static_arr_info res = {true, data, sz / 8}; 
#ifdef DBG
	uint64_t tmp;
	for(size_t i = 0; i < sz / 8; i++){
		tmp = res.bp[i];
		canonicalize_b(&tmp);
		if(tmp != res.bp[i]){
			log_out("Reading non canonicalized board!!!!", LOG_WARN);
		}
	}
	
#endif
	return res;
fail:
	logf_out("Couldn't read %s!", LOG_WARN, dir);
	return (static_arr_info){.valid = false};
}

void read_boards2(static_arr_info *b, const char *dir){
	*b = read_boards(dir);
}

double pun_uint64(uint64_t num){
	return *(double*)(&num);
}
