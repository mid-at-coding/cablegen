#include "../inc/solve.h"
#include "../inc/logging.h"
#include "../inc/board.h"
#include "../inc/generate.h"
#include "../inc/settings.h"
#include <math.h>
#include <stdio.h>
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
	t->key =   init_sarr(0, sz / 16);
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

double lookup(uint64_t key, table *t, bool canonicalize){
	if(t->key.size == 0){
		log_out("Empty table!", LOG_TRACE_);
		return 0.0;
	}
	// binary search for the index of the key
	if(canonicalize)
		canonicalize_b(&key);
	int current_depth = 0;	
	int max_depth = (log(t->key.size) / log(2)) + 1; // add an extra iteration for safety
	size_t top = t->key.size;
	size_t bottom = 0;
	size_t midpoint = (top + bottom) / 2;
	while (t->key.bp[midpoint] != key){
		LOGIF(LOG_TRACE_){
			printf("Current midpoint: %ld, %016lx(%ld)\n", midpoint, t->key.bp[midpoint], t->key.bp[midpoint]);
		}
		if(current_depth > max_depth){
			log_out("Couldn't find board!", LOG_WARN_);
			LOGIF(LOG_TRACE_){
				printf("Current midpoint: %ld, %016lx(%ld)\n", midpoint, t->key.bp[midpoint], t->key.bp[midpoint]);
			}
			return 0.0;
		}
		if(t->key.bp[midpoint] < key){
			bottom = midpoint;
			LOGIF(LOG_TRACE_){
				printf("t->key.bp[%ld] (%ld) < key (%ld)\n", midpoint, t->key.bp[midpoint], key);
			}
		}
		else{
			top = midpoint;
			LOGIF(LOG_TRACE_){
				printf("t->key.bp[%ld] (%ld) >= key (%ld)\n", midpoint, t->key.bp[midpoint], key);
			}
		}
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

void solve(unsigned start, unsigned end, char *posfmt, char *tablefmt, static_arr_info *initial_winstates, unsigned cores){
	set_log_level(LOG_INFO_);
	threadpool th = threadpool_t_init(cores);
	const size_t FILENAME_SIZE = 100;
	bool free_formation = 0;
	get_bool_setting("free_formation", &free_formation);
	dynamic_arr_info winstates_d = init_darr(0,0);
	generate_lut(free_formation); // if we don't gen a lut we can't move
	// generate all rotations of the winstates
	for(size_t i = 0; i < initial_winstates->size; i++){
		uint64_t *rots = get_all_rots(initial_winstates->bp[i]);
		for(int j = 0; j < 8; j++){ // loop over all 8 symmetries
			push_back(&winstates_d, rots[j]);
		}
	}
	static_arr_info winstates = shrink_darr(&winstates_d);
	for(size_t i = 0; i < winstates.size; i++){
		LOGIF(LOG_DBG_){
			printf("winstates %ld: %016lx\n", i, winstates.bp[i]);
		}
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
	for(unsigned int i = start; i >= end; i -= 2){
		snprintf(filename, FILENAME_SIZE, posfmt, i);
		n->key = read_boards(filename);
		n->value = init_sarr(0,n->key.size);
		solve_layer(n4, n2, n, &winstates, cores, th);
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

static bool cmpbrd(uint64_t board, uint64_t board2){
	for(int i = 0; i < 16; i++){
		if((GET_TILE(board2, i)) == 0) // 0s 
			continue;
		else if(GET_TILE(board, i) != GET_TILE(board2, i)){
			return false;	
		}
	}
	return true;
}

static bool satisfied(uint64_t *board, static_arr_info *winstates){
/*	for(int i = 0; i < 16; i++){
		if(GET_TILE((*board), i) == 0x8){
			return true;
		}
	}
	return false; */
	for(size_t i = 0; i < winstates->size; i++){
		if(cmpbrd(*board, winstates->bp[i])){
			return true;
		}
	}
	return false;
}
static double maxmove(uint64_t board, table *n, static_arr_info *winstates){
	uint64_t tmp;
	double prob = 0;
	for(dir d = left; d <= down; d++){
		tmp = board;
		if(movedir(&tmp, d)){
			if(satisfied(&tmp, winstates))
				return 1.0;
			prob = fmax(prob, lookup(tmp, n, true));
		}
	}
	return prob;
}
double expectimax(uint64_t board, table *n2, table *n4, static_arr_info *winstates){
	int spaces = 0;
	double n2prob = 0;
	double n4prob = 0;
	for(short i = 0; i < 16; i++){
		if(GET_TILE(board, i))
			continue;
		++spaces;
		SET_TILE(board, i, 1);
		n2prob += maxmove(board, n2, winstates);
		SET_TILE(board, i, 2);
		n4prob += maxmove(board, n4, winstates);
		SET_TILE(board, i, 0);
	}
	if(spaces == 0) {
		log_out("No space!", LOG_TRACE_);
		return 0;
	};
	return (0.9 * n2prob / spaces) + (0.1 * n4prob / spaces);
}

void* solve_worker_thread(void *args){
	solve_core_data *sargs = args;
	for(size_t curr = sargs->start; curr < sargs->end; curr++){
		double prob = 0;
		if(satisfied(&sargs->n->key.bp[curr], sargs->winstates)){
			prob = 1;
			log_out("Winstate!", LOG_TRACE_);
		}
		else
			prob = expectimax(sargs->n->key.bp[curr], sargs->n2, sargs->n4, sargs->winstates); // we should not be moving -- we're reading moves
		sargs->n->value.bp[curr] = *((uint64_t*)(&prob));
	}
	return NULL;
}

void solve_layer(table *n4, table *n2, table *n, static_arr_info *winstates, unsigned core_count, threadpool pool){
	solve_core_data *cores = malloc_errcheck(sizeof(solve_core_data) * core_count);
	for(unsigned i = 0; i < core_count; i++){ // initialize worker threads
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
		threadpool_add_work(pool, solve_worker_thread, (void*)(cores + i));
	}
	threadpool_wait(pool);
}
