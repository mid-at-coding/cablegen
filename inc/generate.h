#ifndef GENERATE_H
#define GENERATE_H
#include <stdint.h>
#include <pthread.h>
#include "array.h"

struct{
	static_arr_info two;
	static_arr_info four;
} typedef spawn_ret;

spawn_ret spawnarr(const static_arr_info, dynamic_arr_info*);
uint64_t* movearr(const static_arr_info, dynamic_arr_info*);
void generate_layer(const char* file, dynamic_arr_info* n, dynamic_arr_info* n2, dynamic_arr_info* n4, dynamic_arr_info* potential_duplicate, const uint);
void generate(const int, const int, const char*, uint64_t*, const size_t, const uint);

#endif
