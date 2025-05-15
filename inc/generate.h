#ifndef GENERATE_H
#define GENERATE_H
#include <stdint.h>
#include <pthread.h>
#include "array.h"
#include "../inc/cthreadpool.h"
typedef threadpool_t* threadpool;

void generate(const int, const int, const char*, uint64_t*, const size_t, const unsigned, bool prespawn);
static_arr_info read_boards(const char *dir);
void write_boards(const static_arr_info n, const char* fmt, const int layer);

#endif
