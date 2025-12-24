#ifndef GENERATE_H
#define GENERATE_H
#include <stdint.h>
#include "array.h"

void generate(const int start, const int end, const char *fmt, const static_arr_info *initial);
static_arr_info read_boards(const char *dir);
void write_boards(const static_arr_info n, const char* fmt, const int layer);
bool checkx(uint64_t board, char x);
void print_speed(size_t size);

#endif
