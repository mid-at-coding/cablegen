#ifndef BENCH_H
#include <stdio.h>
#include <time.h>
#define BENCH_H
#define BENCH_NODE_LIST \
	X(GEN_LAYER) \
	X(SOLVE_LAYER) \
	X(MOVE) \
	X(SPAWN) \
	X(GEN_MOVE) \
	X(GEN_SPAWN) \
	X(SORT_MOVE) \
	X(SORT_SPAWN) \
	X(DEDUPE_MOVE) \
	X(DEDUPE_SPAWN) \
	X(COMBINE_SPAWN) \
	X(COMBINE_MOVE) \
	X(WRITE)

struct interval{
	struct timespec start;
	struct timespec end;
};
extern FILE *bench_file;
extern int bench_layer;
extern struct interval node_times[
#define X(name) 1+
BENCH_NODE_LIST 0
#undef X
];

typedef enum {
#define X(name) name,
	BENCH_NODE_LIST
#undef X
} bench_node;

char *get_bench_node_name(const bench_node);
void set_layer(int);
void open_bench(char *file, char *name);
void start_node(const bench_node);
void end_node(const bench_node);
void end_node_n(const bench_node, const long long);
void end_bench();

#endif
