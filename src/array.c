#include "array.h"
#include "logging.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#define SORT_NAME uint64
#define SORT_TYPE uint64_t
#include "sort.h"

bool push_back(dynamic_arr_info *info, uint64_t v){
	bool res = false;
#ifndef NOERRCHECK
	if(info->sp == NULL){
		log_out("Invalid stack pointer, refusing to push\n", LOG_WARN_);
		info->valid = false;
		return false;
	}
	if(info->valid == false){
		log_out("Invalid array, refusing to push\n", LOG_WARN_);
		return false;
	}
#endif
	if(info->sp == info->bp + info->size){
		// reallocate
		size_t size = info->sp - info->bp;
		info->bp = realloc(info->bp, info->size ? (info->size * REALLOC_MULT * sizeof(uint64_t)) : sizeof(uint64_t));
#ifndef NOERRCHECK
		if (info->bp == NULL){
			log_out("Alloc failed! Download more ram!\n", LOG_ERROR_);
			info->valid = false;
			return false;
		}
#endif
		if(info->size)
			info->size *= REALLOC_MULT;
		else
			info->size = 1;
		info->sp = info->bp + size;
		res = true;
	}
	*(info->sp++) = v;
	return res;
}

dynamic_arr_info init_darr(bool zero, size_t size){
	dynamic_arr_info d;
	d.valid = true;
	d.bp = NULL;
	d.sp = NULL;
	d.size = 0;
	if(zero)
		d.bp = calloc(size, sizeof(uint64_t));
	else	
		d.bp = malloc(sizeof(uint64_t) * size);
	if (d.bp == NULL){
		log_out("Alloc failed! Download more ram!\n", LOG_ERROR_);
		d.valid = false;
	}
	d.sp = d.bp;
	d.size = size;
	return d;
}

static_arr_info init_sarr(bool zero, size_t size){
	static_arr_info s;
	s.valid = true;
	if(zero)
		s.bp = calloc(size, sizeof(uint64_t));
	else	
		s.bp = malloc(sizeof(uint64_t) * size);
	if (s.bp == NULL){
		log_out("Alloc failed! Download more ram!\n", LOG_ERROR_);
		s.valid = false;
	}
	s.size = size;
	return s;
}

static_arr_info shrink_darr(dynamic_arr_info* info){
	int new_size = info->sp - info->bp;
#ifndef NOERRCHECK
	if(info->valid == false){
		log_out("Invalid array, refusing to shrink\n", LOG_WARN_);
		return (static_arr_info){.valid = false, .bp = info->bp, .size = info->sp - info->bp};
	}
#endif
	info->valid = false; 
	if(new_size == 0){
		free(info->bp);
		info->bp = NULL;
		info->sp = NULL;
		info->size = 0;
	}
	uint64_t *new_bp = realloc(info->bp, sizeof(uint64_t) * new_size);
#ifndef NOERRCHECK
	if(new_bp == NULL){
		log_out("Shrink failed!\n", LOG_ERROR_);
		return (static_arr_info){.valid = false, .bp = info->bp, .size = info->sp - info->bp};
	}
#endif
	return (static_arr_info){.valid = true, .bp = new_bp, .size = new_size};
}
dynamic_arr_info concat(dynamic_arr_info * restrict arr1, dynamic_arr_info * restrict arr2){
#ifndef NOERRCHECK
	if(!arr1->valid || !arr2->valid){
		log_out("Refusing to concatenate invalid arrays!", LOG_WARN_);
		return *arr1;
	}
#endif
	arr1->valid = arr2->valid = false;
	dynamic_arr_info arr1_dynamic = {
		.valid = true,
		.bp   = NULL,
		.sp   = NULL,
		.size = (arr1->sp - arr1->bp) + (arr2->sp - arr2->bp)
	};
	if(arr1->size >= arr1_dynamic.size){
		arr1_dynamic.bp = arr1->bp;
	}
	else{
		arr1_dynamic.bp = malloc_errcheck(arr1_dynamic.size * sizeof(uint64_t));
		memcpy(arr1_dynamic.bp, arr1->bp, (arr1->sp - arr1->bp) * sizeof(uint64_t));
		free(arr1->bp);
	}
	arr1_dynamic.sp = arr1_dynamic.bp + arr1_dynamic.size;
	memcpy(arr1_dynamic.bp + (arr1->sp - arr1->bp), arr2->bp, (arr2->sp - arr2->bp) * sizeof(uint64_t));
	free(arr2->bp);
	return arr1_dynamic;
}
dynamic_arr_info concat_unique(dynamic_arr_info * restrict arr1, dynamic_arr_info * restrict arr2){
	static_arr_info arr1_shrink = shrink_darr(arr1);
	static_arr_info arr2_shrink = shrink_darr(arr2);
	arr1->valid = arr2->valid = false;
	dynamic_arr_info arr1_dynamic = {
		.valid = true,
		.bp   = ((arr1_shrink.bp == NULL) ? init_darr(0,false).bp : arr1_shrink.bp ),
		.sp   = arr1_shrink.bp + arr1_shrink.size, 
		.size = arr1_shrink.size
	};
	for(size_t i = 0; i < arr2_shrink.size; i++){
		bool in_arr = false;
		for(uint64_t *v = arr1_dynamic.bp; v < arr1_dynamic.sp; v++)
			if(arr2_shrink.bp[i] == *v)
				in_arr = true;
		if(!in_arr)
			push_back(&arr1_dynamic, arr2_shrink.bp[i]);
	}
	free(arr2_shrink.bp);
	return arr1_dynamic;
}

void deduplicate(dynamic_arr_info *s){ // TODO optimize this somehow
    if(s->sp == s->bp || s-> sp == s->bp + 1){
		log_out("Can't sort one value!\n", LOG_DBG_);
		return;
	}
	size_t size = s->sp - s->bp;
    dynamic_arr_info res = init_darr(0, 0.7 * size); // assume it's around 30% dupes
	uint64_tim_sort(s->bp, size);
	push_back(&res, *s->bp);
	for(uint64_t *curr = s->bp + 1; curr < s->sp; curr++){
		if(*curr != *(curr - 1)){
			push_back(&res, *curr);
		}
	}
	destroy_darr(s);
	*s = res;
	return;
}

void deduplicate_qs(dynamic_arr_info *s){
    if(s->sp == s->bp || s-> sp == s->bp + 1){
		log_out("Can't sort one value!\n", LOG_DBG_);
		return;
	}
	size_t size = s->sp - s->bp;
    dynamic_arr_info res = init_darr(0, 0.7 * size); // assume it's around 30% dupes
	uint64_quick_sort(s->bp, size);
	push_back(&res, *s->bp);
	for(uint64_t *curr = s->bp + 1; curr < s->sp; curr++){
		if(*curr != *(curr - 1)){
			push_back(&res, *curr);
		}
	}
	destroy_darr(s);
	*s = res;
	return;
}

void* malloc_errcheck(size_t size){ // guaranteed to be non-null
#ifndef NOERRCHECK
	void* res = malloc(size);
	if(res == NULL){
		log_out("Alloc failed!", LOG_ERROR_);
		exit(EXIT_FAILURE);
		return NULL;
	}
	return res;
#endif
#ifdef NOERRCHECK
	return malloc(size);
#endif
}
dynamic_arr_info sarrtodarr(static_arr_info *s){
	dynamic_arr_info tmp;
	tmp.bp = s->bp;
	tmp.sp = s->bp + s->size;
	tmp.valid = s->valid;
	tmp.size = s->size;
	return tmp;
}
void destroy_darr(dynamic_arr_info* arr){
	free(arr->bp);
	arr->valid = false;
}
void destroy_sarr(static_arr_info* arr){
	free(arr->bp);
	arr->valid = false;
}

void init_buckets(buckets *b){
	for(size_t i = 0; i < BUCKETS_N; i++){
		b->bucket[i].d = init_darr(0,0);
		pthread_mutex_init(&b->bucket[i].mut, NULL);
	}
}

void destroy_buckets(buckets *b){
	for(size_t i = 0; i < BUCKETS_N; i++){
		destroy_darr(&b->bucket[i].d);
		if(pthread_mutex_destroy(&b->bucket[i].mut)){
			log_out("Could not destroy bucket mutex!", LOG_WARN_);
		}
	}
}

_BitInt(BUCKETS_DIGITS) get_first_digits(uint64_t tmp){
	static uint64_t mask = 0;
#if BUCKETS_DIGITS > 64
#warn "are you sure"
#endif
	if(mask == 0){ // TODO: may have a race condition
		for(int i = 0; i < BUCKETS_DIGITS; i++){
			mask = mask | (1 >> i); 
		}
	}
	return mask & tmp;
}

bool push_back_into_bucket(buckets *b, uint64_t d){
	_BitInt(BUCKETS_DIGITS) radix = get_first_digits(d);
	pthread_mutex_lock(&b->bucket[radix].mut);
	bool res = push_back(&b->bucket[radix].d, d); // TODO: if this function takes a while the mutex can be added to darrs for TCO
	pthread_mutex_unlock(&b->bucket[radix].mut);
	return res;
}
