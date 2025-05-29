#ifndef SOLVE_H
#define SOLVE_H
#include "../inc/array.h"
#include "../inc/cthreadpool.h"

typedef threadpool_t* threadpool;
typedef struct {
	static_arr_info key;
	static_arr_info value;
} table;

void write_table(const table *t, const char *path);
void read_table(table *t, const char *path);
void solve(unsigned start, unsigned end, char *posfmt, char *tablefmt, static_arr_info *winstates, unsigned cores, char nox, bool score);
void solve_layer(table *n4, table *n2, table *n, static_arr_info *winstataes, unsigned core_count, threadpool pool, char nox, bool score);
double lookup(uint64_t key, table *t, bool canonicalize);
double expectimax(uint64_t board, table *n2, table *n4, static_arr_info *winstates, char nox, bool score);


#endif
