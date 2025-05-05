#ifndef SOLVE_H
#define SOLVE_H
#include "../inc/array.h"
#include "../inc/thpool.h"

typedef struct {
	static_arr_info key;
	static_arr_info value;
} table;

void write_table(const table *t, const char *path);
void read_table(table *t, const char *path);
void solve(unsigned start, unsigned end, char *posfmt, char *tablefmt, static_arr_info *winstates, unsigned cores);
void solve_layer(table *n4, table *n2, table *n, static_arr_info *winstataes, unsigned core_count, threadpool pool);
double lookup(uint64_t key, table *t);


#endif
