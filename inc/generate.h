#ifndef GENERATE_H
#define GENERATE_H
#include <stdint.h>
#include <pthread.h>
#include "array.h"
#include "../inc/cthreadpool.h"
typedef threadpool_t* threadpool;

void generate(const int start, const int end, const char *fmt, uint64_t *initial, const size_t initial_len, const unsigned, bool premove, char nox, bool free_formation);
static_arr_info read_boards(const char *dir);
void write_boards(const static_arr_info n, const char* fmt, const int layer);
bool checkx(uint64_t board, char x);

#endif
