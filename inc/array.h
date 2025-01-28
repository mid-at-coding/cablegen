#ifndef ARR_H
#define ARR_H
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#define REALLOC_MULT 2

struct{
	uint64_t* bp;
	uint64_t* sp;
	size_t size;
	pthread_mutex_t mut;
} typedef dynamic_arr_info;

struct{
	uint64_t* bp;
	size_t size;
} typedef static_arr_info;

[[nodiscard]] dynamic_arr_info init_darr(bool zero, size_t size); // error handling is callee's responsibility
[[nodiscard]] static_arr_info init_sarr(bool zero, size_t size); // error handling is callee's responsibility
[[nodiscard]] static_arr_info shrink_darr(dynamic_arr_info* info); // passed dynamic array becomes invalid
[[nodiscard]] dynamic_arr_info concat(dynamic_arr_info* arr1, dynamic_arr_info* arr2); // both arrays become invalid
[[nodiscard]] dynamic_arr_info concat_unique(dynamic_arr_info* arr1, dynamic_arr_info* arr2); // both arrays become invalid, returned array has exactly one instance of each value in arr2
bool push_back(dynamic_arr_info*, uint64_t); // returns whether a resize happened
#endif
