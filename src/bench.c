#include "bench.h"
#include <bits/time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#define LOG_H_ENUM_PREFIX_
#define LOG_H_NAMESPACE_ 
#include "logging.h"
#include <stdio.h>

FILE *bench_file;
int bench_layer;
struct interval node_times[
#define X(name) 1+
BENCH_NODE_LIST 0
#undef X
];

char *get_bench_node_name(const bench_node n){
	switch(n){
#define X(name) case name: return #name;
	BENCH_NODE_LIST
#undef X
	}
	return "";
}
void set_layer(int l){
	bench_layer = l;
}
void open_bench(char *file, char *name){
	bench_file = fopen(file, "w");
	if(!bench_file){
		logf_out("Could not open %s for benchmarking! Error: %s", LOG_ERROR, file, strerror(errno));
		exit(EXIT_FAILURE);
	}
	fprintf(bench_file, "digraph %s{\n", name);
}
void start_node(const bench_node node){
	clock_gettime(CLOCK_MONOTONIC, &node_times[node].start);
}
static void connect(const bench_node node){
	switch(node){
		case GEN_LAYER:
			fprintf(bench_file, "\tGEN_LAYER%d -> GEN_LAYER%d // gen_layer\n", bench_layer, bench_layer - 2);
			break;
		case SOLVE_LAYER:
			fprintf(bench_file, "\tGEN_LAYER%d -> SOLVE_LAYER%d // solve_layer\n", bench_layer, bench_layer);
			break;
		case MOVE:
			fprintf(bench_file, "\tGEN_LAYER%d -> MOVE%d // move\n", bench_layer - 2, bench_layer);
			break;
		case SPAWN:
			fprintf(bench_file, "\tMOVE%d -> SPAWN%d // spawn\n", bench_layer, bench_layer);
			break;
		case SORT_MOVE:
			fprintf(bench_file, "\tGEN_MOVE%d -> SORT_MOVE%d // sort_move\n", bench_layer, bench_layer);
			break;
		case SORT_SPAWN:
			fprintf(bench_file, "\tGEN_SPAWN%d -> SORT_SPAWN%d // sort_spawn\n", bench_layer, bench_layer);
			break;
		case GEN_MOVE:
			fprintf(bench_file, "\tGEN_MOVE%d -> MOVE%d // gen_move\n", bench_layer, bench_layer);
			break;
		case GEN_SPAWN:
			fprintf(bench_file, "\tGEN_SPAWN%d -> SPAWN%d // gen_spawn\n", bench_layer, bench_layer);
			break;
		case DEDUPE_MOVE:
			fprintf(bench_file, "\tCOMBINE_MOVE%d -> DEDUPE_MOVE%d // dedupe_move\n", bench_layer, bench_layer);
			break;
		case DEDUPE_SPAWN:
			fprintf(bench_file, "\tCOMBINE_SPAWN%d -> DEDUPE_SPAWN%d // dedupe_spawn\n", bench_layer, bench_layer);
			break;
		case WRITE:
			fprintf(bench_file, "\tDEDUPE_SPAWN%d -> WRITE%d // write\n", bench_layer, bench_layer);
			break;
		case COMBINE_MOVE:
			fprintf(bench_file, "\tGEN_MOVE%d -> COMBINE_MOVE%d // combine_move\n", bench_layer, bench_layer);
			break;
		case COMBINE_SPAWN:
			fprintf(bench_file, "\tGEN_SPAWN%d -> COMBINE_SPAWN%d // combine_spawn\n", bench_layer, bench_layer);
			break;
	}
}
long getms(const struct timespec start, const struct timespec end){
	struct timespec diff = { 
		.tv_sec  = end.tv_sec  - start.tv_sec,
		.tv_nsec = end.tv_nsec - start.tv_nsec 
	};
	return ((1'000'000'000 * diff.tv_sec) + diff.tv_nsec) / 1000000;
}
void end_node(const bench_node node){
	clock_gettime(CLOCK_MONOTONIC, &node_times[node].end);
	long totalms = getms(node_times[node].start, node_times[node].end);
	fprintf(bench_file, "\t%s%d [label=\"%s %d t(ms): %ld\"];\n", get_bench_node_name(node), bench_layer, 
			get_bench_node_name(node), bench_layer, totalms);
	connect(node);
}
void end_node_n(const bench_node node, const long long n){
	clock_gettime(CLOCK_MONOTONIC, &node_times[node].end);
	long totalms = getms(node_times[node].start, node_times[node].end);
	fprintf(bench_file, "\t%s%d [label=\"%s %d t(ms): %ld\", n: %lld];\n", get_bench_node_name(node), bench_layer, 
			get_bench_node_name(node), bench_layer, totalms, n);
	connect(node);
}
void end_bench(){
	fprintf(bench_file, "}\n");
	fclose(bench_file);
}
