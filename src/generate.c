#include "generate.h"
#include "format.h"
#include <bits/time.h>
#define LOG_H_ENUM_PREFIX_
#define LOG_H_NAMESPACE_ 
#include "logging.h"
#include "board.h"
#include "settings.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#ifdef BENCH
	struct timespec startT, endT;
	size_t startC, endC;
	int clayer;
#endif
#define STR(x) #x
#define EXPAND_STR(x) STR(x)
#define VERSION_STR EXPAND_STR(VERSION)
#ifdef BENCH
FILE *bench_output;
#endif

typedef struct {
	static_arr_info n; 
	dynamic_arr_info nret;
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
enum thread_op {
	move,
	movep,
	spawn,
};

void print_speed(uint64_t size){
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
	size_t filename_size = strlen(fmt) + 10; // if there are more than 10 digits of layers i'll eat my shoe
	char* filename = malloc_errcheck(sizeof(char) * filename_size);
	snprintf(filename, filename_size, fmt, layer);
	logf_out("Writing %lu boards to %s (%lu bytes)", LOG_INFO, n.size, filename, sizeof(uint64_t) * n.size);  // lol
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
	short tmp = 0; // TODO: masking
	short large_tiles = 0;
	int smallest = 0xff;
	int sts = 0; // small tile sum
	uint16_t tiles = 0;
	uint16_t tiles2 = 0;
	char c64 = 0;
	for(short i = 0; i < 16; i++){
		if((tmp = GET_TILE(board, i)) >= smallest_large && tmp < 0xe){
			smallest = tmp > smallest ? smallest : tmp;
			large_tiles++;
			if (tmp == smallest_large && c64 < 3)
				c64++;
			else if (tmp == smallest_large && c64 == 2)
				return true;
			else if(!GETBIT(tiles, tmp))
				SETBIT(tiles, tmp);
			else if(!GETBIT(tiles2, tmp))
				SETBIT(tiles2, tmp);
			else
				return true;
		}
		else if(tmp < 0xe){
			sts += 1 << tmp;
		}
	}
	if(sts > stsl + 64)
		return true;
	if(large_tiles > ltc)
		return true;
	// condition number three seems impossible??
	return false;
}

void *generation_thread_move(void* data){ // n, nret
	arguments *args = data;
	uint64_t tmp;
	uint64_t old;
	for(size_t i = args->start; i < args->end; i++){
		old = args->n.bp[i];
		tmp = old;
		for(dir d = left; d <= down; d++){
			if(movedir_unstable(&tmp, d)){
				canonicalize_b(&tmp); // TODO it's not necessary to gen *all* boards in nox
				
				push_back(&args->nret, tmp);
				tmp = old;
			}
		}
	}
	deduplicate_qs(&args->nret);
	return NULL;
}

void *generation_thread_movep(void* data){ // n, nret, stsl, ltc, smallest_large
	arguments *args = data;
	uint64_t tmp;
	uint64_t old;
	for(size_t i = args->start; i < args->end; i++){
		old = args->n.bp[i];
		if(prune_board(old, args->stsl, args->ltc, args->smallest_large))
			continue;
		tmp = old;
		for(dir d = left; d <= down; d++){
			if(movedir_unstable(&tmp, d)){
				if(prune_board(tmp, args->stsl, args->ltc, args->smallest_large))
					continue;
				canonicalize_b(&tmp);
				push_back(&args->nret, tmp);
				tmp = old;
			}
		}
	}
	deduplicate_qs(&args->nret);
	return NULL;
}

void *generation_thread_spawn(void* data){
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
	deduplicate_qs(&args->n2); // TODO optimize
	deduplicate_qs(&args->n4);
	return NULL;
}

static void init_threads(const dynamic_arr_info *n, const unsigned int core_count, enum thread_op op, arguments *cores, char nox, long layer){ 
	// TODO make these ops work with solving too?
	void *(*fn)(void*);
	switch(op){
	case movep:
		fn = generation_thread_movep;
		break;
	case move:
		fn = generation_thread_move;
		break;
	case spawn:
		fn = generation_thread_spawn;
		break;
	}
	for(unsigned i = 0; i < core_count; i++){ // initialize worker threads
		cores[i].n = (static_arr_info){.valid = n->valid, .bp = n->bp, .size = n->sp - n->bp};
		switch(op){
		case movep:
			cores[i].stsl = get_settings()->stsl;
			cores[i].ltc = get_settings()->ltc;
		case move:
			cores[i].nret = init_darr(0, 3 * (n->sp - n->bp) / core_count);
			break;
		case spawn:
			cores[i].n2 = init_darr(0, 4 * (n->sp - n->bp) / core_count);
			cores[i].n4 = init_darr(0, 4 * (n->sp - n->bp) / core_count);
			cores[i].nox = nox;
			break;
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
	}
	for(unsigned i = 0; i < core_count; i++){
		[[maybe_unused]] int e;
		log_out("Creating thread", LOG_TRACE);
		if((e = pthread_create(&cores[i].thread, NULL, fn, (void*)(cores + i)))){
#ifndef NOERRCHECK
			log_out("Failed creating thread!", LOG_ERROR);
			exit(EXIT_FAILURE);
#endif
		}
	}
}

static void wait(arguments *cores, size_t core_count){
	for(size_t i = 0; i < core_count; i++){
		pthread_join(cores[i].thread, NULL);
	}
}

static void replace_n(dynamic_arr_info *n, arguments *cores, const unsigned int core_count){
	wait(cores, core_count);
#ifdef BENCH
	clock_gettime(CLOCK_MONOTONIC, &endT);
	struct timespec mergeS, mergeE;
	clock_gettime(CLOCK_MONOTONIC, &mergeS);
#endif
	destroy_darr(n);
	*n = init_darr(0, 0);
	for(size_t i = 0; i < core_count; i++){
		*n = concat(n, &cores[i].nret);
	}
#ifdef BENCH
	clock_gettime(CLOCK_MONOTONIC, &mergeE);
	struct timespec diff = { 
		.tv_sec  = endT.tv_sec  - startT.tv_sec,
		.tv_nsec = endT.tv_nsec - startT.tv_nsec };
	long totalns = (1'000'000'000 * diff.tv_sec) + diff.tv_nsec;
	endC = n->sp - n->bp;
	logf_f(bench_output, "\tm%d [label=\"Moving (N: %ld, T(ms): %ld)\"];", LOG_NONE, clayer, startC - endC, totalns / 1000);
	logf_f(bench_output, "\tl%d -- m%d;", LOG_NONE, clayer, clayer);
	diff = (struct timespec){ 
		.tv_sec  = mergeE.tv_sec  - mergeS.tv_sec,
		.tv_nsec = mergeE.tv_nsec - mergeS.tv_nsec };
	totalns = (1'000'000'000 * diff.tv_sec) + diff.tv_nsec;
	logf_f(bench_output, "\tmergem%d [label=\"Merging moves (T(ms): %ld)\"];", LOG_NONE, clayer, totalns / 1000);
	logf_f(bench_output, "\tm%d -- mergem%d;", LOG_NONE, clayer, clayer);
#endif
}

void generate_layer(dynamic_arr_info* n, dynamic_arr_info* n2, dynamic_arr_info* n4, 
		const unsigned core_count, const char *fmt_dir, const int layer, arguments *cores, char nox){
	// move
#ifdef BENCH
	startC = n->sp - n->bp;
	clayer = layer;
	clock_gettime(CLOCK_MONOTONIC, &startT);
#endif
	if(get_settings()->prune)
		init_threads(n, core_count, movep, cores, nox, layer);
	else
		init_threads(n, core_count, move, cores, nox, layer);
	// wait for moves to be done
	replace_n(n, cores, core_count); // this array currently holds boards where we just spawned -- these are never our responsibility
	deduplicate(n);
	// spawn
#ifdef BENCH
	startC = n2->sp - n2->bp + n4->sp - n4->bp;
	clock_gettime(CLOCK_MONOTONIC, &startT);
#endif
	init_threads(n, core_count, spawn, cores, nox, layer);
	// write while waiting for spawns
#ifdef BENCH
	struct timespec writeS, writeE;
	clock_gettime(CLOCK_MONOTONIC, &writeS);
#endif
	write_boards((static_arr_info){.valid = n->valid, .bp = n->bp, .size = n->sp - n->bp}, fmt_dir, layer);
#ifdef BENCH
	clock_gettime(CLOCK_MONOTONIC, &writeE);
#endif
	wait(cores,core_count);
#ifdef BENCH
	clock_gettime(CLOCK_MONOTONIC, &endT);
	struct timespec diff = { 
		.tv_sec  = endT.tv_sec  - startT.tv_sec,
		.tv_nsec = endT.tv_nsec - startT.tv_nsec };
	long totalns = (1'000'000'000 * diff.tv_sec) + diff.tv_nsec;
	startC = n2->sp - n2->bp + n4->sp - n4->bp;
	logf_f(bench_output, "\ts%d [label=\"Spawning (N: %ld, T(ms): %ld)\"];", LOG_NONE, clayer, startC - endC, totalns / 1000);
	logf_f(bench_output, "\tl%d -- s%d;", LOG_NONE, clayer, clayer);
	diff = (struct timespec){ 
		.tv_sec  = writeE.tv_sec  - writeS.tv_sec,
		.tv_nsec = writeE.tv_nsec - writeS.tv_nsec };
	totalns = (1'000'000'000 * diff.tv_sec) + diff.tv_nsec;
	logf_f(bench_output, "\twrite%d [label=\"Writing spawns (T(ms): %ld)\"];", LOG_NONE, clayer, totalns / 1000);
	logf_f(bench_output, "\ts%d -- write%d;", LOG_NONE, clayer, clayer);
#endif
#ifdef BENCH
	clock_gettime(CLOCK_MONOTONIC, &startT);
#endif
	// concatenate spawns
	for(size_t i = 0; i < core_count; i++){
		*n2 = concat(n2, &cores[i].n2);
		*n4 = concat(n4, &cores[i].n4);
	}
#ifdef BENCH
	clock_gettime(CLOCK_MONOTONIC, &endT);
	diff = (struct timespec){ 
		.tv_sec  = endT.tv_sec  - startT.tv_sec,
		.tv_nsec = endT.tv_nsec - startT.tv_nsec };
	totalns = (1'000'000'000 * diff.tv_sec) + diff.tv_nsec;
	logf_f(bench_output, "\tmerges%d [label=\"Merging spawns (T(ms): %ld)\"];", LOG_NONE, clayer, totalns / 1000);
	logf_f(bench_output, "\ts%d -- merges%d;", LOG_NONE, clayer, clayer);
	clock_gettime(CLOCK_MONOTONIC, &startT);
#endif
	deduplicate(n4);
	deduplicate(n2);
#ifdef BENCH
	clock_gettime(CLOCK_MONOTONIC, &endT);
	diff = (struct timespec){ 
		.tv_sec  = endT.tv_sec  - startT.tv_sec,
		.tv_nsec = endT.tv_nsec - startT.tv_nsec };
	totalns = (1'000'000'000 * diff.tv_sec) + diff.tv_nsec;
	logf_f(bench_output, "\tdds%d [label=\"Deduping spawns (T(ms): %ld)\"];", LOG_NONE, clayer, totalns / 1000);
	logf_f(bench_output, "\ts%d -- dds%d;", LOG_NONE, clayer, clayer);
#endif
}
void generate(const int start, const int end, const char* fmt, uint64_t* initial, const size_t initial_len, 
		const unsigned core_count, bool prespawn, char nox, bool free_formation){
	// GENERATE: write all sub-boards where it is the computer's move	
#ifdef BENCH
	char *bench_output_name = format_str("%s.bench.gv", VERSION_STR);
	bench_output = fopen(bench_output_name, "w");    
	if (!bench_output){
		logf_out("Couldn't open file %s for benchmarking performance, sending to stdout instead", LOG_WARN, bench_output_name);
		bench_output = stdout;
	}
	logf_f(bench_output, "// Benchmarking generation", LOG_NONE);
	free(bench_output_name);
#endif
	generate_lut();
	static const size_t DARR_INITIAL_SIZE = 100;
	dynamic_arr_info n  = init_darr(false, 0);
	free(n.bp);
	n.bp = initial;
	n.size = initial_len;
	n.sp = n.size + n.bp;
	dynamic_arr_info n2 = init_darr(false, DARR_INITIAL_SIZE);
	dynamic_arr_info n4 = init_darr(false, DARR_INITIAL_SIZE);
	arguments *cores = malloc_errcheck(sizeof(arguments) * core_count);
	if(prespawn){
		arguments prespawn_args;
		prespawn_args.n = (static_arr_info){.valid = n.valid, .bp = n.bp, .size = n.sp - n.bp};
		prespawn_args.n2 = n2;
		prespawn_args.n4 = n4;
		prespawn_args.start = 0;
		prespawn_args.end = prespawn_args.n.size;
		generation_thread_spawn(&prespawn_args);
	}
#ifdef BENCH
	logf_f(bench_output, "graph generation{\n", LOG_NONE);
	logf_f(bench_output, "\troot [label=\"Generation\"];", LOG_NONE);
#endif
	for(int i = start; i <= end; i += 2){
		generate_layer(&n, &n2, &n4, core_count, fmt, i, cores, nox);
		destroy_darr(&n);
#ifdef BENCH
		logf_f(bench_output, "\tl%d [label=\"Layer %d (n=%d)\"];", LOG_NONE, i, i, n2.sp - n2.bp + n4.sp - n4.bp);
//		logf_f(bench_output, "\tl%d -- root;", LOG_NONE, i, i);
#endif
		n = n2;
		n2 = n4;
		n4 = init_darr(false, DARR_INITIAL_SIZE);
	}
#ifdef BENCH
	logf_f(bench_output, "}\n", LOG_NONE);
	fflush(bench_output);
#endif
	destroy_darr(&n);
	destroy_darr(&n2);
	destroy_darr(&n4);
	free(cores);
#ifdef BENCH
	if(bench_output != stdout){ // don't close stdout xp
		fclose(bench_output);
	}
#endif
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
