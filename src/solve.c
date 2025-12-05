#include "solve.h"
#define LOG_H_ENUM_PREFIX_
#define LOG_H_NAMESPACE_ 
#include "logging.h"
#include "board.h"
#include "generate.h"
#include "settings.h"
#include <assert.h>
#include <math.h>
#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
typedef struct {
	long layer;
	char nox;
	bool score;
	dynamic_arr_info nret;
	table *n;
	table *n2;
	table *n4;
	size_t start;
	size_t end;
	static_arr_info *winstates;
	pthread_t thread;
} solve_core_data;
void write_table(const table *t, const char *filename){ // TODO fix the speed situation
	logf_out("Writing %lu boards to %s (%lu bytes)", LOG_INFO, t->key.size, filename, 2 * sizeof(uint64_t) * t->key.size);
	FILE *file = fopen(filename, "wb");
	if(file == NULL){
		logf_out("Couldn't write to %s!", LOG_WARN, filename);
		return;
	}
	if(t->key.size != t->value.size){
		logf_out("Key and value size inequal, refusing to write! (%lu, %lu)", LOG_WARN, t->key.size, t->value.size);
		fclose(file);
		return;
	}
	fwrite(t->key.bp,   sizeof(uint64_t), t->key.size,   file);
	fwrite(t->value.bp, sizeof(double),   t->value.size, file);
	fclose(file);
	print_speed(t->key.size);
}

void read_table(table *t, const char *filename){ // TODO clean up this and read_boards
	FILE *fp = fopen(filename, "rb");
	size_t res = 0;
	if(fp == NULL){
		logf_out("Couldn't read %s!", LOG_WARN, filename);
		t->key = init_sarr(0,0);
		t->value = init_sarr(0,0);
		t->key.valid = false;
		t->value.valid = false;
		return;
	}
	errno = 0;
	fseek(fp, 0L, SEEK_END);
	if(errno){
		goto read_err;
	}
	errno = 0;
	size_t sz = ftell(fp);
	if(errno){
		goto read_err;
	}
	errno = 0;
	rewind(fp);
	if(errno){
		goto read_err;
	}
	if(sz % 16 != 0) // 16 is 2 * 8 bytes is a double and a board
		log_out("sz %%16 != 0, this is probably not a real table!", LOG_WARN);
	t->key =   init_sarr(0, sz / 16);
	t->value = init_sarr(0, sz / 16);
	res = fread(t->key.bp, sizeof(uint64_t), sz / 16, fp);
	if(ferror(fp) || res != sz / 16){
		log_out("Error reading file!", LOG_WARN);
		fclose(fp);
		t->key.valid = false;
		t->value.valid = false;
		return;
	}
	res = fread(t->value.bp, sizeof(double), sz / 16, fp);
	if(ferror(fp) || res != sz / 16){
		log_out("Error reading file!", LOG_WARN);
		fclose(fp);
		t->key.valid = false;
		t->value.valid = false;
		return;
	}
	logf_out("Read %ld bytes (%ld boards) from %s", LOG_INFO, sz, sz / 16, filename);
	fclose(fp);
	return;
read_err:
	log_out("Failed getting size of table!", LOG_ERROR);
	fclose(fp);
	t->key = init_sarr(0,0);
	t->value = init_sarr(0,0);
	t->key.valid = false;
	t->value.valid = false;
	return;
#ifdef DBG
#endif
}

double lookup(uint64_t key, table *t, bool canonicalize){ // TODO get rid of these defines and make this nicer to adjust (+ test)
	if(t->key.size == 0){
		log_out("Empty table!", LOG_TRACE);
		return 0.0;
	}
	// binary search for the index of the key
	if(canonicalize)
		canonicalize_b(&key);
	static const int SEARCH_STOP = 50;
	size_t top = t->key.size;
	size_t bottom = 0;
	size_t midpoint = (top + bottom) / 2;
	// use binary search tree cache
	while (t->key.bp[midpoint] != key){
#ifdef DBG
		logf_out("Current midpoint: %ld, %016lx(%ld)", LOG_TRACE, midpoint, t->key.bp[midpoint], t->key.bp[midpoint]);
#endif
		if(top - bottom < SEARCH_STOP){
#ifdef DBG
			log_out("Switching to linear search", LOG_TRACE);
#endif
			for(size_t i = bottom; i < top; i++){
#ifdef DBG
				logf_out("Current board: %ld, %016lx(%ld)", LOG_TRACE, i, t->key.bp[i], t->key.bp[i]);
#endif
				if(t->key.bp[i] == key){
#ifdef DBG
					log_out("Found board!", LOG_TRACE);
#endif
					// return value as a double
					return *(double*)(&(t->value.bp[i]));
				}
			}
#ifdef DBG
			log_out("Couldn't find board!", LOG_TRACE);
			logf_out("board: %016lx", LOG_TRACE, key);
#endif
			return 0.0;
		}
		if(t->key.bp[midpoint] < key){
			bottom = midpoint;
#ifdef DBG
			logf_out("t->key.bp[%ld] (%ld) < key (%ld)", LOG_TRACE, midpoint, t->key.bp[midpoint], key);
#endif
		}
		else{
			top = midpoint;
#ifdef DBG
			logf_out("t->key.bp[%ld] (%ld) >= key (%ld)", LOG_TRACE, midpoint, t->key.bp[midpoint], key);
#endif
		}
		midpoint = (top + bottom) / 2;
//		current_depth++;
	}
	// return value as a double
	return *(double*)(&(t->value.bp[midpoint]));
}

void destroy_table(table* t){
	free(t->key.bp);
	free(t->value.bp);
	free(t);
}

void solve(unsigned start, unsigned end, char *posfmt, char *tablefmt, static_arr_info *initial_winstates, unsigned cores, char nox, 
		bool score, bool free_formation){
	const size_t FILENAME_SIZE = 100;
	dynamic_arr_info winstates_d = init_darr(0,0);
	generate_lut(); // if we don't gen a lut we can't move
	// generate all required rotations of the winstates
//	bool *rots_required = required_symmetries(initial_winstates);
	for(size_t i = 0; i < initial_winstates->size; i++){
		uint64_t *rots = get_all_rots(initial_winstates->bp[i]);
		for(int j = 0; j < 8; j++){ // loop over all 8 symmetries
//			if(!rots_required[j]) broken 
//				continue;
			push_back(&winstates_d, rots[j]);
		}
		free(rots);
	}
	static_arr_info winstates = shrink_darr(&winstates_d);
	for(size_t i = 0; i < winstates.size; i++){
		logf_out("winstates %ld: %016lx", LOG_DBG, i, winstates.bp[i]);
	}
	table *n4 = malloc_errcheck(sizeof(table));
	table *n2 = malloc_errcheck(sizeof(table));
	table *n  = malloc_errcheck(sizeof(table));
	table *tmp_p;
	n2->key   = init_sarr(0,0);
	n2->value = init_sarr(0,0);
	n4->key   = init_sarr(0,0);
	n4->value = init_sarr(0,0);
	char *filename = malloc_errcheck(FILENAME_SIZE);
	for(unsigned int i = start; i >= end && i; i -= 2){
		snprintf(filename, FILENAME_SIZE, posfmt, i);
		n->key = read_boards(filename);
		if(get_settings()->delete_boards)
			remove(filename);
		n->value = init_sarr(0,n->key.size);
		solve_layer(n4, n2, n, &winstates, cores, nox, score, i);
		snprintf(filename, FILENAME_SIZE, tablefmt, i);
		write_table(n, filename);
		// cycle the tables -- n goes down by two so n2 will be our freshly solved n, n4 our read n2, and n will be reset next loop
		free(n4->key.bp);
		free(n4->value.bp);
		tmp_p = n4;
		n4    = n2;
		n2    = n;
		n     = tmp_p;
	}
	free(winstates.bp);
	destroy_table(n2);
	destroy_table(n4);
	free(filename);
}

bool cmpbrd(uint64_t board, uint64_t board2){
	uint64_t board_mask = (board2 >> 2) | board2;
	board_mask = ((board_mask >> 1) | board_mask) & 0x1111111111111111;
	board_mask = board_mask * 0xf;
	return (board & board_mask) == board2;
}

bool satisfied(const uint64_t *board, const static_arr_info *winstates, const char nox, const bool score){
	if(nox && !score)
		return !checkx(*board, nox);
	if(score)
		return 0;
	for(size_t i = 0; i < winstates->size; i++){
		if(cmpbrd(*board, winstates->bp[i])){
			return true;
		}
	}
	return false;
}

static double maxmove(uint64_t board, table *n, static_arr_info *winstates, char nox, bool score){
	uint64_t tmp;
	double prob[4] = {0};
	for(dir d = left; d <= down; d++){
		tmp = board;
		if(movedir_unstable(&tmp, d)){
			prob[d] = lookup(tmp, n, true);
		}
	}
	return fmax(fmax(prob[0], prob[1]), fmax(prob[2], prob[3]));
}
double expectimax(uint64_t board, table *n2, table *n4, static_arr_info *winstates, char nox, bool score){
	int spaces = 0;
	double n2prob = 0;
	double n4prob = 0;
	for(short i = 0; i < 16; i++){
		if(GET_TILE(board, i))
			continue;
		++spaces;
		SET_TILE(board, i, 1);
		n2prob += maxmove(board, n2, winstates, nox, score);
		SET_TILE(board, i, 2);
		n4prob += maxmove(board, n4, winstates, nox, score);
		SET_TILE(board, i, 0);
	}
	double res = 0;
	if(spaces == 0)
		log_out("No space!", LOG_TRACE);
	else
		res = (0.9 * n2prob / spaces) + (0.1 * n4prob / spaces);
	if(score && res == 0){ // if this board is a leaf or otherwise dead, return the score. this is the base case
		// this is technically incorrect
		res = get_sum(board);
	}
	return res;
}

static void wait(solve_core_data *cores, size_t core_count){
	for(size_t i = 0; i < core_count; i++){
		pthread_join(cores[i].thread, NULL);
	}
}

void *solve_worker_thread(void *args){
	solve_core_data *sargs = args;
	for(size_t curr = sargs->start; curr < sargs->end; curr++){
		double prob = 0;
		if(satisfied(&sargs->n->key.bp[curr], sargs->winstates, sargs->nox, sargs->score)){
			prob = 1;
			log_out("Winstate!", LOG_TRACE);
		}
		else
			prob = expectimax(sargs->n->key.bp[curr], sargs->n2, sargs->n4, sargs->winstates, sargs->nox, sargs->score); // we should not be moving -- we're reading moves
		sargs->n->value.bp[curr] = *((uint64_t*)(&prob));
	}
	return NULL;
}

enum solve_op{
	op_solve,
	op_prune
};

void init_threads(table *n, table *n2, table *n4, static_arr_info *winstates, unsigned core_count, char nox, bool score, 
		long layer, solve_core_data *cores, enum solve_op op){
	for(unsigned i = 0; i < core_count; i++){ // initialize worker threads
		cores[i].n = n;
		if(op == op_solve){
			cores[i].n2 = n2;
			cores[i].n4 = n4;
			cores[i].nox = nox;
			cores[i].score = score;
			cores[i].winstates = winstates;
		}
		// divide up [0,n.size)
		// cores work in [start,end)
		int block_size = (n->key.size) / core_count;
		cores[i].start = i * block_size;
		cores[i].end = (i + 1) * block_size;
		// core_count * n.size / core_count = n.size
		// make sure that the last thread covers all of the array
		if(i + 1 == core_count){
			cores[i].end = n->key.size;
		}
		if(op == op_solve)
			pthread_create(&cores[i].thread, NULL, solve_worker_thread, (void*)(cores + i));
	}
}

void solve_layer(table *n4, table *n2, table *n, static_arr_info *winstates, unsigned core_count, char nox, bool score, long layer){
	solve_core_data *cores = malloc_errcheck(sizeof(solve_core_data) * core_count);
	init_threads(n, n2, n4, winstates, core_count, nox, score, layer, cores, op_solve);
	wait(cores, core_count);
	free(cores);
}
