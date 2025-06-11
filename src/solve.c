#include "../inc/solve.h"
#include "../inc/logging.h"
#include "../inc/board.h"
#include "../inc/generate.h"
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <threads.h>
#include <time.h>
typedef struct {
	char nox;
	bool score;
	table *n;
	table *n2;
	table *n4;
	size_t start;
	size_t end;
	static_arr_info *winstates;
	pthread_t thread;
} solve_core_data;
void write_table(const table *t, const char *filename){
	static bool startup_init = 0;
	static time_t old = 0;
	LOGIF(LOG_INFO_){
		printf("[INFO] Writing %lu boards to %s (%lu bytes)\n", t->key.size, filename, 2 * sizeof(uint64_t) * t->key.size);  // lol
	}
	FILE *file = fopen(filename, "wb");
	if(file == NULL){
		char *buf = malloc_errcheck(100);
		snprintf(buf, 100, "Couldn't write to %s!\n", filename);
		log_out(buf, LOG_WARN_);
		free(buf);
		return;
	}
	if(!startup_init){
		startup_init = true;
		old = time(NULL);
	}
	else{
		clock_t curr = clock();
		size_t diff = curr - old;
		old = curr;
		LOGIF(LOG_INFO_){
			printf("[INFO] Speed: %ld thousand boards per second\n", (long)((double)t->key.size / (double)((diff * 1000.0f) / (double)CLOCKS_PER_SEC)));
		}
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

uint64_t next_pow2(uint64_t x) { 	return x == 1 ? 1 : 1<<(64-__builtin_clzl(x-1)); }

double lookup_shar(uint64_t lookup, table *t, bool canonicalize){
	size_t length = t->key.size;
	size_t begin = 0;
	size_t end = t->key.size;
	if(length == 0){
		log_out("Empty table!", LOG_TRACE_);
		return 0.0;
	}
	if(canonicalize)
		canonicalize_b(&lookup);
	size_t step = next_pow2(t->key.size) / 2;
	if(step != length && t->key.bp[step] < lookup){
		length -= step + 1;
		if(length == 0)
			return *((double*)&t->value.bp[end - 1]);
		step = next_pow2(length);
		begin = end - step;
	}
	for(step /= 2; step != 0; step /= 2){
		if(t->key.bp[begin + step] < lookup)
			begin += step;
	}
	return *((double*)&t->value.bp[begin + (t->key.bp[begin] < lookup)]);
}

double lookup(uint64_t key, table *t, bool canonicalize){
//	return lookup_shar(key, t, canonicalize);
	if(t->key.size == 0){
		log_out("Empty table!", LOG_TRACE_);
		return 0.0;
	}
	// binary search for the index of the key
	if(canonicalize)
		canonicalize_b(&key);
	static const int SEARCH_STOP = 50;
//	int current_depth = 0;	
//	int max_depth = (log(t->key.size) / log(2)) + 1; // add an extra iteration for safety
	size_t top = t->key.size;
	size_t bottom = 0;
	size_t midpoint = (top + bottom) / 2;
	while (t->key.bp[midpoint] != key){
		LOGIF(LOG_TRACE_){
			printf("Current midpoint: %ld, %016lx(%ld)\n", midpoint, t->key.bp[midpoint], t->key.bp[midpoint]);
		}
		if(top - bottom < SEARCH_STOP){
			log_out("Switching to linear search", LOG_TRACE_);
			for(size_t i = bottom; i <= top; i++){
				LOGIF(LOG_TRACE_){
					printf("Current board: %ld, %016lx(%ld)\n", i, t->key.bp[i], t->key.bp[i]);
				}
				if(t->key.bp[i] == key){
					log_out("Found board!", LOG_TRACE_);
					// return value as a double
					return *(double*)(&(t->value.bp[i]));
				}
			}
			log_out("Couldn't find board!", LOG_WARN_);
			LOGIF(LOG_WARN_){
				printf("board: %016lx\n", key);
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
		solve_layer(n4, n2, n, &winstates, cores, nox, score);
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

static bool satisfied(uint64_t *board, static_arr_info *winstates, char nox, bool score){
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
	double prob = 0;
	for(dir d = left; d <= down; d++){
		tmp = board;
		if(movedir(&tmp, d)){
			if(satisfied(&tmp, winstates, nox, score))
				return 1.0;
			if((nox && checkx(tmp,nox)) || !nox)
				prob = fmax(prob, lookup(tmp, n, true));
		}
	}
	return prob;
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
		log_out("No space!", LOG_TRACE_);
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
			log_out("Winstate!", LOG_TRACE_);
		}
		else
			prob = expectimax(sargs->n->key.bp[curr], sargs->n2, sargs->n4, sargs->winstates, sargs->nox, sargs->score); // we should not be moving -- we're reading moves
		sargs->n->value.bp[curr] = *((uint64_t*)(&prob));
	}
	return NULL;
}

void solve_layer(table *n4, table *n2, table *n, static_arr_info *winstates, unsigned core_count, char nox, bool score){
	solve_core_data *cores = malloc_errcheck(sizeof(solve_core_data) * core_count);
	for(unsigned i = 0; i < core_count; i++){ // initialize worker threads
		cores[i].n = n;
		cores[i].n2 = n2;
		cores[i].n4 = n4;
		cores[i].nox = nox;
		cores[i].score = score;
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
		pthread_create(&cores[i].thread, NULL, solve_worker_thread, (void*)(cores + i));
	}
	wait(cores, core_count);
	free(cores);
}
