#include "../inc/solve.h"
#include "../inc/logging.h"
#include "../inc/board.h"
#include "../inc/generate.h"
#include <math.h>
#include <stdio.h>

void write_table_solved(const table *t, const char *filename){
	printf("[INFO] Writing %lu boards to %s (%lu bytes)\n", t->key.size, filename, 2 * sizeof(uint64_t) * t->key.size);  // lol
	FILE *file = fopen(filename, "wb");
	if(file == NULL){
		char *buf = malloc_errcheck(100);
		snprintf(buf, 100, "Couldn't write to %s!\n", filename);
		log_out(buf, LOG_WARN_);
		free(buf);
		return;
	}
	if(t->key.size != t->value.size){
		log_out("Key and value size inequal, refusing to write!", LOG_WARN_);
		return;
	}
	fwrite(t->key.bp,   t->key.size,   sizeof(uint64_t), file);
	fwrite(t->value.bp, t->value.size, sizeof(uint64_t), file);
	fclose(file);
}

void read_table_solved(table *t, const char *filename){
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
	if(sz % 16 != 0) // 16 is 2 * 8 bytes 
		log_out("sz %%16 != 0, this is probably not a real table!\n", LOG_WARN_);
	t->key = init_darr(0, sz / 2);
	t->value = init_darr(0, sz / 2);
	fread(t->key.bp,   1, sz / 2, fp);
	fread(t->value.bp, 1, sz / 2, fp);
	char *buf = malloc_errcheck(100);
	snprintf(buf, 100, "Read %ld bytes (%ld boards) from %s\n", sz, sz / 16, filename);
	log_out(buf, LOG_DBG_);
	free(buf);
	fclose(fp);
}

double lookup(uint64_t key, table *t){
	// binary search for the index of the key
	int current_depth = 0;	
	int max_depth = (log(t->key.sp - t->key.bp) / log(2)) + 1; // when we'll stop searching
	size_t top = t->key.sp - t->key.bp;
	size_t bottom = 0;
	size_t midpoint = (top + bottom) / 2;
	while (t->key.bp[midpoint] != key){
		if(current_depth > max_depth)
			return 0.0;
		if(t->key.bp[midpoint] > key)
			bottom = midpoint;
		else
			top = midpoint;
		midpoint = (top + bottom) / 2;
		current_depth++;
	}

	// return value as a double
	return *(double*)(&(t->value.bp[midpoint]));
}

void solve(unsigned start, unsigned end, char *posfmt, char *tablefmt){
	table *n4 = malloc_errcheck(sizeof(table));
	table *n2 = malloc_errcheck(sizeof(table));
	table *n = malloc_errcheck(sizeof(table));
	char *filename = malloc_errcheck(100);
	for(unsigned int i = start; i >= end; i -= 2){
		snprintf(filename, 100, posfmt, i);
		static_arr_info tmp = read_table(filename);
		n->key = sarrtodarr(&tmp);
		snprintf(filename, 100, tablefmt, i + 2);
		read_table_solved(n2, filename);
		snprintf(filename, 100, tablefmt, i + 4);
		read_table_solved(n4, filename);
		solve_layer(n4, n2, n);
		snprintf(filename, 100, tablefmt, i);
		write_table_solved(n, filename);
	}
}

double expectimax(uint64_t board, table *n2, table *n4){
	int spaces = 0;
	double n2prob = 0;
	double n4prob = 0;
	for(short i = 0; i < 16; i++){
		if(!GET_TILE(board, i)){
			++spaces;
			SET_TILE(board, i, 1);
			n2prob += lookup(board, n2);
			SET_TILE(board, i, 1);
			n4prob += lookup(board, n4);
		}
	}
	return (0.9 * n2prob / spaces) + (0.1 * n4prob / spaces);
}

void solve_layer(table *n4, table *n2, table *n){
	for(uint64_t *curr = n->key.bp; curr < n->key.sp; curr++){
		// this current board is on our turn so we wanna take max of all moves
		double max = 0;
		double tmp = 0;
		for(dir d = left; d < down; d++){
			if(movedir(curr, d)){ // if we can move in a direction, find the expected win%
				if((tmp = expectimax(*curr, n2, n4)) > max)
					max = tmp;
			}
		}
		push_back(&(n->value), *((uint64_t*)(&max)));
	}
}
