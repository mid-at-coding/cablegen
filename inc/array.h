#ifndef ARR_H
#define ARR_H
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#define REALLOC_MULT 3

typedef struct{
	bool valid;
	uint64_t* bp;
	uint64_t* sp;
	size_t size;
} dynamic_arr_info;

typedef struct{
	bool valid;
	uint64_t* bp;
	size_t size;
} static_arr_info;
void* malloc_errcheck(size_t size); // guaranteed to be non-null
dynamic_arr_info init_darr(bool zero, size_t size); // error handling is callee's responsibility
static_arr_info init_sarr(bool zero, size_t size); // error handling is callee's responsibility
static_arr_info shrink_darr(dynamic_arr_info* info); // passed dynamic array becomes invalid
dynamic_arr_info concat(dynamic_arr_info* arr1, dynamic_arr_info* arr2); // both arrays become invalid
dynamic_arr_info concat_unique(dynamic_arr_info* arr1, dynamic_arr_info* arr2); // both arrays become invalid, returned array has exactly one instance of each value in arr2
void destroy_darr(dynamic_arr_info* arr); // frees and invalidates array
void destroy_sarr(static_arr_info* arr); // frees and invalidates array
bool push_back(dynamic_arr_info*, uint64_t); // returns whether a resize happened
void deduplicate(dynamic_arr_info*);
void deduplicate_masked(dynamic_arr_info*);
dynamic_arr_info sarrtodarr(static_arr_info*);
void destroy_darr(dynamic_arr_info* arr);
void destroy_sarr(static_arr_info* arr);
#endif
