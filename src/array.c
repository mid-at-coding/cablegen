#include "../inc/array.h"
#include "../inc/logging.h"
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#define SORT_NAME uint64
#define SORT_TYPE uint64_t
#include "../inc/sort.h"

bool push_back(dynamic_arr_info *info, uint64_t v){
	bool res = false;
	if(info->valid == false){
		log_out("Invalid array, refusing to push\n", LOG_WARN_);
		return false;
	}
	if(info->sp == info->bp + info->size){
		// reallocate
		size_t size = info->sp - info->bp;
		info->bp = realloc(info->bp, info->size ? (info->size * REALLOC_MULT * sizeof(uint64_t)) : sizeof(uint64_t));
		if (info->bp == NULL){
			log_out("Alloc failed! Download more ram!\n", LOG_ERROR_);
			info->valid = false;
		}
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

[[nodiscard]] dynamic_arr_info init_darr(bool zero, size_t size){
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

[[nodiscard]] static_arr_info init_sarr(bool zero, size_t size){
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

[[nodiscard]] static_arr_info shrink_darr(dynamic_arr_info* info){
	int new_size = info->sp - info->bp;
	if(info->valid == false){
		log_out("Invalid array, refusing to shrink\n", LOG_WARN_);
		exit(1);
		return (static_arr_info){.valid = false, .bp = info->bp, .size = info->sp - info->bp};
	}
	info->valid = false; 
	if(new_size == 0){
		free(info->bp);
		info->bp = NULL;
		info->sp = NULL;
		info->size = 0;
	}
	uint64_t *new_bp = realloc(info->bp, sizeof(uint64_t) * new_size);
	if(new_bp == NULL){
		log_out("Shrink failed!\n", LOG_ERROR_);
		return (static_arr_info){.valid = false, .bp = info->bp, .size = info->sp - info->bp};
	}
	return (static_arr_info){.valid = true, .bp = new_bp, .size = new_size};
}
[[nodiscard]] dynamic_arr_info concat(dynamic_arr_info * restrict arr1, dynamic_arr_info * restrict arr2){
	arr1->valid = arr2->valid = false;
	dynamic_arr_info arr1_dynamic = {
		.valid = true,
		.bp   = NULL,
		.sp   = NULL,
		.size = (arr1->sp - arr1->bp) + (arr2->sp - arr2->bp)
	};
	arr1_dynamic.bp = malloc_errcheck(arr1_dynamic.size * sizeof(uint64_t));
	arr1_dynamic.sp = arr1_dynamic.bp + arr1_dynamic.size;
	memcpy(arr1_dynamic.bp, arr1->bp, (arr1->sp - arr1->bp) * sizeof(uint64_t));
	memcpy(arr1_dynamic.bp + (arr1->sp - arr1->bp), arr2->bp, (arr2->sp - arr2->bp) * sizeof(uint64_t));
	free(arr1->bp);
	free(arr2->bp);
	return arr1_dynamic;
}
[[nodiscard]] dynamic_arr_info concat_unique(dynamic_arr_info * restrict arr1, dynamic_arr_info * restrict arr2){
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

static int compare(const void *a, const void *b){
    return (((*(uint64_t*)a) > (*(uint64_t*)b))) - (((*(uint64_t*)a) < (*(uint64_t*)b)));
}

struct qsort_args {
	uint64_t* bp;
	size_t size;
	size_t index;
};

static void qsort_wt(void *args){
	struct qsort_args *qargs = args;
	uint64_quick_sort(qargs->bp, qargs->size);
}

void deduplicate_wt(void *vargs){
	deduplicate_args *args = vargs;
	deduplicate(args->d, 1, NULL);
}

void deduplicate(dynamic_arr_info *s, size_t core_count, threadpool pool){
    if(s->sp == s->bp || s-> sp == s->bp + 1){
		log_out("Can't sort one value!\n", LOG_WARN_);
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

[[nodiscard]] void* malloc_errcheck(size_t size){ // guaranteed to be non-null
	void* res = malloc(size);
	if(res == NULL){
		log_out("Alloc failed!", LOG_ERROR_);
		exit(ENOMEM);
		return NULL;
	}
	return res;
}
dynamic_arr_info sarrtodarr(static_arr_info *s){
	dynamic_arr_info tmp = init_darr(0,1);
	free(tmp.bp); // hacky
	tmp.bp = s->bp;
	tmp.sp = s->bp + s->size;
	tmp.valid = s->valid && tmp.valid;
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
