#include "../inc/solve.h"
#include "../inc/logging.h"
#include "../inc/board.h"
#include "../inc/generate.h"
#include "../inc/settings.h"
#include <math.h>
#include <stdio.h>
#define DBG
void write_table(const table *t, const char *filename){
	printf("[INFO] Writing %lu boards to %s (%lu bytes)\n", t->key.size, filename, 2 * sizeof(uint64_t) * t->key.size);  // lol
	FILE *file = fopen(filename, "wb");
	if(file == NULL){
		char *buf = malloc_errcheck(100);
		snprintf(buf, 100, "Couldn't write to %s!\n", filename);
		log_out(buf, LOG_WARN_);
		free(buf);
		return;
	}
	if(t->key.sp - t->key.bp != t->value.sp - t->value.bp){
		log_out("Key and value size inequal, refusing to write!", LOG_WARN_);
		printf("(%lu, %lu)/n", t->key.sp - t->key.bp, t->value.sp - t->value.bp);
		return;
	}
	fwrite(t->key.bp,   t->key.sp   - t->key.bp,   sizeof(uint64_t), file);
	fwrite(t->value.bp, t->value.sp - t->value.bp, sizeof(uint64_t), file);
	fclose(file);
}

void read_table(table *t, const char *filename){
	FILE *fp = fopen(filename, "rb");
	if(fp == NULL){
		char *buf = malloc_errcheck(100);
		snprintf(buf, 100, "Couldn't read %s!\n", filename);
		t->key = init_darr(0,0);
		t->value = init_darr(0,0);
		log_out(buf, LOG_WARN_);
		free(buf);
		return;
	}
	fseek(fp, 0L, SEEK_END);
	size_t sz = ftell(fp);
	rewind(fp);
	if(sz % 16 != 0) // 16 is 2 * 8 bytes is a double and a board
		log_out("sz %%16 != 0, this is probably not a real table!\n", LOG_WARN_);
	t->key = init_darr(0, sz / 2);
	t->value = init_darr(0, sz / 2);
	fread(t->key.bp,   1, sz / 2, fp);
	fread(t->value.bp, 1, sz / 2, fp);
	char *buf = malloc_errcheck(100);
	snprintf(buf, 100, "Read %ld bytes (%ld boards) from %s\n", sz, sz / 16, filename);
	if(t->key.sp - t->key.bp != t->value.sp - t->value.bp){
		log_out("Key and value size inequal, this is definitely not a real table!", LOG_WARN_);
		t->key.valid = false;
		t->value.valid = false;
		return;
	}
	log_out(buf, LOG_DBG_);
	free(buf);
	fclose(fp);
#ifdef DBG
#endif
}

double lookup(uint64_t key, table *t){
	// binary search for the index of the key
	canonicalize(&key);
	int current_depth = 0;	// TODO switch this to use pointers instead of indexes, may be faster?
	int max_depth = (log(t->key.sp - t->key.bp) / log(2)) + 1; // when we'll stop searching
	size_t top = t->key.sp - t->key.bp;
	size_t bottom = 0;
	size_t midpoint = (top + bottom) / 2;
	while (t->key.bp[midpoint] != key){
		if(current_depth > max_depth){
			log_out("Couldn't find board!", LOG_TRACE_);
			return 0.0;
		}
		if(t->key.bp[midpoint] < key)
			bottom = midpoint;
		else
			top = midpoint;
		midpoint = (top + bottom) / 2;
		current_depth++;
	}
	log_out("Found board!", LOG_TRACE_);
	// return value as a double
	return *(double*)(&(t->value.bp[midpoint]));
}

void solve(unsigned start, unsigned end, char *posfmt, char *tablefmt, static_arr_info *winstates){
	set_log_level(LOG_INFO_);
	const size_t FILENAME_SIZE = 100;
	bool free_formation = 0;
	get_bool_setting("free_formation", &free_formation);
	generate_lut(free_formation); // if we don't gen a lut we can't move
	// make sure winstates are canonicalized otherwise they won't work right
	for(size_t i = 0; i < winstates->size; i++)
		canonicalize(winstates->bp + i);
	table *n4 = malloc_errcheck(sizeof(table));
	table *n2 = malloc_errcheck(sizeof(table));
	table *n  = malloc_errcheck(sizeof(table));
	n2->key = init_darr(0,0);
	n2->value = init_darr(0,0);
	n4->key = init_darr(0,0);
	n4->value = init_darr(0,0);
	char *filename = malloc_errcheck(FILENAME_SIZE);
	for(unsigned int i = start; i >= end; i -= 2){
		snprintf(filename, FILENAME_SIZE, posfmt, i);
		static_arr_info tmp = read_boards(filename);
		n->key = sarrtodarr(&tmp);
		n->value = init_darr(0,0);
		solve_layer(n4, n2, n, winstates);
		snprintf(filename, FILENAME_SIZE, tablefmt, i);
		write_table(n, filename);
		// cycle the tables -- n goes down by two so n2 will be our freshly solved n, n4 our read n2, and n will be reset next loop
		destroy_darr(&n4->key);
		destroy_darr(&n4->value);
		free(n4);
		n4 = n2;
		n2 = n;
		n  = malloc_errcheck(sizeof(table));
	}
}

static bool satisfied(uint64_t *board, static_arr_info *winstates){
	for(int i = 0; i < winstates->size; i++){
		canonicalize(board);
		if(((winstates->bp[i]) & (*board)) == *board){
			return true;
		}
	}
	return false;
}
static double expectimaxm(uint64_t board, table *n){
	uint64_t tmp = board;
	int c = 0;
	double prob = 0;
	for(dir d = left; d < down; d++){
		if(movedir(&tmp, d)){
			++c;
			prob += lookup(tmp, n);
		}
	}
	return (c == 0) ? 0 : prob / c;
}
double expectimax(uint64_t board, table *n2, table *n4, static_arr_info *winstates){
	int spaces = 0;
	double n2prob = 0;
	double n4prob = 0;
	for(short i = 0; i < 16; i++){
		if(!GET_TILE(board, i)){
			++spaces;
			SET_TILE(board, i, 1);
			n2prob += satisfied(&board, winstates) ? 1 : expectimaxm(board, n2);
			SET_TILE(board, i, 2);
			n4prob += satisfied(&board, winstates) ? 1 : expectimaxm(board, n4);
			SET_TILE(board, i, 0);
		}
	}
	if(spaces == 0) {
		log_out("No space!", LOG_TRACE_);
		return 0;
	};
	return (0.9 * n2prob / spaces) + (0.1 * n4prob / spaces);
}

void solve_layer(table *n4, table *n2, table *n, static_arr_info *winstates){
	for(uint64_t *curr = n->key.bp; curr < n->key.sp; curr++){
		double prob = 0;
		if(satisfied(curr, winstates)){
			prob = 1;
		}
		else{
			prob = expectimax(*curr, n2, n4, winstates); // we should not be moving -- we're reading moves
		}
		push_back(&(n->value), *((uint64_t*)(&prob)));
	}
}
