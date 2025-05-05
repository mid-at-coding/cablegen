#include "../inc/solve.h"
#include "../inc/logging.h"
#include "../inc/board.h"
#include "../inc/generate.h"
#include "../inc/settings.h"
#include <math.h>
#include <stdio.h>
#define DBG
typedef struct {
	table *n;
	table *n2;
	table *n4;
	size_t start;
	size_t end;
	static_arr_info *winstates;
} solve_core_data;
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
	if(t->key.size != t->value.size){
		log_out("Key and value size inequal, refusing to write!", LOG_WARN_);
		printf("(%lu, %lu)/n", t->key.size, t->value.size);
		return;
	}
	fwrite(t->key.bp,   sizeof(uint64_t), t->key.size,   file);
	fwrite(t->value.bp, sizeof(double),   t->value.size, file);
	fclose(file);
}

void read_table(table *t, const char *filename){
	FILE *fp = fopen(filename, "rb");
	if(fp == NULL){
		char *buf = malloc_errcheck(100);
		snprintf(buf, 100, "Couldn't read %s!\n", filename);
		t->key = init_sarr(0,0);
		t->value = init_sarr(0,0);
		log_out(buf, LOG_WARN_);
		free(buf);
		return;
	}
	fseek(fp, 0L, SEEK_END);
	size_t sz = ftell(fp);
	rewind(fp);
	if(sz % 16 != 0) // 16 is 2 * 8 bytes is a double and a board
		log_out("sz %%16 != 0, this is probably not a real table!\n", LOG_WARN_);
	t->key = init_sarr(0, sz / 16);
	t->value = init_sarr(0, sz / 16);
	fread(t->key.bp,   sizeof(uint64_t), sz / 16, fp);
	fread(t->value.bp, sizeof(double),   sz / 16, fp);
	char *buf = malloc_errcheck(100);
	snprintf(buf, 100, "Read %ld bytes (%ld boards) from %s\n", sz, sz / 16, filename);
	log_out(buf, LOG_DBG_);
	free(buf);
	fclose(fp);
#ifdef DBG
#endif
}

double lookup(uint64_t key, table *t){
//	printf("size: %d\n", t->key.size);
//	for(size_t i = 0; i < t->key.size; i++){
//		printf("looking: %lx , %d\n", t->key.bp[i], i);
//	}
	if(t->key.size == 0){
		log_out("Empty table!", LOG_TRACE_);
		return 0.0;
	}
	// binary search for the index of the key
	canonicalize_b(&key);
	int current_depth = 0;	
	int max_depth = (log(t->key.size) / log(2)) + 1; // when we'll stop searching
	size_t top = t->key.size;
	size_t bottom = 0;
	size_t midpoint = (top + bottom) / 2;
	while (t->key.bp[midpoint] != key){
//		printf("looking: %lx , %d\n", t->key.bp[midpoint], midpoint);
		if(current_depth > max_depth){
			log_out("Couldn't find board!", LOG_TRACE_);
			printf("%lx\n", key);
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

void destroy_table(table* t){
	free(t->key.bp);
	free(t->value.bp);
	free(t);
}

void solve(unsigned start, unsigned end, char *posfmt, char *tablefmt, static_arr_info *winstates, unsigned cores){
	set_log_level(LOG_INFO_);
	threadpool th = thpool_init(cores);
	const size_t FILENAME_SIZE = 100;
	bool free_formation = 0;
	get_bool_setting("free_formation", &free_formation);
	generate_lut(free_formation); // if we don't gen a lut we can't move
	// make sure winstates are canonicalized otherwise they won't work right
	for(size_t i = 0; i < winstates->size; i++)
		canonicalize_b(winstates->bp + i);
	table *n4 = malloc_errcheck(sizeof(table));
	table *n2 = malloc_errcheck(sizeof(table));
	table *n  = malloc_errcheck(sizeof(table));
	table *tmp_p;
	n2->key   = init_sarr(0,0);
	n2->value = init_sarr(0,0);
	n4->key   = init_sarr(0,0);
	n4->value = init_sarr(0,0);
	char *filename = malloc_errcheck(FILENAME_SIZE);
	for(unsigned int i = start; i >= end; i -= 2){
		snprintf(filename, FILENAME_SIZE, posfmt, i);
		n->key = read_boards(filename);
		n->value = init_sarr(0,n->key.size);
		solve_layer(n4, n2, n, winstates, cores, th);
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
	destroy_table(n2);
	destroy_table(n4);
}

static bool satisfied(uint64_t *board, static_arr_info *winstates){
	for(int i = 0; i < winstates->size; i++){
		canonicalize_b(board);
		if(((winstates->bp[i]) & (*board)) == *board){
			return true;
		}
	}
	return false;
}
static double expectimaxm(uint64_t board, table *n){
	uint64_t tmp = board;
	double tmp_prob = 0;
	double prob = 0;
	for(dir d = left; d < down; d++){
		if(movedir(&tmp, d)){
			if(prob < (tmp_prob = lookup(tmp, n))){
				prob = tmp_prob;
			}
		}
	}
	return prob;
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

void solve_worker_thread(void *args){
	solve_core_data *sargs = args;
	for(size_t curr = sargs->start; curr < sargs->end; curr++){
		double prob = 0;
		if(satisfied(&sargs->n->key.bp[curr], sargs->winstates)){
			prob = 1;
		}
		else{
			prob = expectimax(sargs->n->key.bp[curr], sargs->n2, sargs->n4, sargs->winstates); // we should not be moving -- we're reading moves
		}
		sargs->n->value.bp[curr] = *((uint64_t*)(&prob));
	}
}

void solve_layer(table *n4, table *n2, table *n, static_arr_info *winstates, unsigned core_count, threadpool pool){
	solve_core_data *cores = malloc_errcheck(sizeof(solve_core_data) * core_count);
	for(uint i = 0; i < core_count; i++){ // initialize worker threads
		cores[i].n = n;
		cores[i].n2 = n2;
		cores[i].n4 = n4;
		cores[i].winstates = winstates;
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
		thpool_add_work(pool, solve_worker_thread, (void*)(cores + i));
	}
	thpool_wait(pool);
}
