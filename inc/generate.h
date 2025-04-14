#ifndef GENERATE_H
#define GENERATE_H
#include <stdint.h>
#include <pthread.h>
#include "array.h"

void generate(const int, const int, const char*, uint64_t*, const size_t, const uint, bool prespawn);
static_arr_info read_boards(const char *dir);

#endif
