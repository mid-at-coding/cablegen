#ifndef SOLVE_H
#define SOLVE_H
#include "../inc/array.h"

typedef struct {
	static_arr_info key;
	static_arr_info value;
} table;

void write_table(const table *t, const char *path);
void read_table(table *t, const char *path);
void solve(unsigned start, unsigned end, char *posfmt, char *tablefmt, const static_arr_info *winstates);
void solve_layer(table *n4, table *n2, table *n, static_arr_info *winstataes, unsigned core_count, char nox, bool score, long layer);
double lookup(uint64_t key, table *t, bool canonicalize);
double expectimax(uint64_t board, table *n2, table *n4, static_arr_info *winstates, char nox, bool score);
bool satisfied(const uint64_t *board, const static_arr_info *winstates, const char nox, const bool score);
bool cmpbrd(uint64_t board, uint64_t board2);

#endif
