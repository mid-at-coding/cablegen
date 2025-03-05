#include "../inc/array.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

bool push_back(dynamic_arr_info *info, uint64_t v){
	pthread_mutex_lock(&info->mut);
	bool res = false;
	if(info->sp == info->bp + info->size){
		// reallocate
		uint64_t *tmp = info->bp;
		info->bp = realloc(info->bp, info->size ? (info->size * REALLOC_MULT * sizeof(uint64_t)) : sizeof(uint64_t));
		if (info->bp == NULL){
			printf("Alloc failed! Download more ram!\n");
			exit(1);
		}
		if(info->size)
			info->size *= REALLOC_MULT;
		else
			info->size = 1;
		info->sp = info->bp + (info->sp - tmp);
		res = true;
	}
	*(info->sp++) = v;
	pthread_mutex_unlock(&info->mut);
	return res;
}

[[nodiscard]] dynamic_arr_info init_darr(bool zero, size_t size){
	dynamic_arr_info d;
	if(zero)
		d.bp = calloc(sizeof(uint64_t), size);
	else	
		d.bp = malloc(sizeof(uint64_t) * size);
	if (d.bp == NULL){
		printf("Alloc failed! Download more ram!\n");
		exit(1);
	}
	int res = pthread_mutex_init(&d.mut, NULL);
	if (res != 0){
		printf("Dynamic arr mutex initialization failed, errcode: %d", res);
		exit(res);
	}
	d.sp = d.bp;
	d.size = size;
	return d;
}

[[nodiscard]] static_arr_info init_sarr(bool zero, size_t size){
	static_arr_info s;
	if(zero)
		s.bp = calloc(sizeof(uint64_t), size);
	else	
		s.bp = malloc(sizeof(uint64_t) * size);
	if (s.bp == NULL){
		printf("Alloc failed! Download more ram!\n");
		exit(1);
	}
	s.size = size;
	return s;
}

[[nodiscard]] static_arr_info shrink_darr(dynamic_arr_info* info){
	int new_size = info->sp - info->bp;
	if(new_size == 0){
		free(info->bp);
		info->bp = NULL;
		info->sp = NULL;
		info->size = 0;
	}
	uint64_t *new_bp = realloc(info->bp, sizeof(uint64_t) * new_size);
	if(new_bp == NULL){
		printf("Shrink failed!\n");
		exit(1);
	}
	return (static_arr_info){new_bp, new_size};
}
[[nodiscard]] dynamic_arr_info concat(dynamic_arr_info * restrict arr1, dynamic_arr_info * restrict arr2){
	static_arr_info arr1_shrink = shrink_darr(arr1);
	static_arr_info arr2_shrink = shrink_darr(arr2);
	dynamic_arr_info arr1_dynamic = {((arr1_shrink.bp == NULL) ? init_darr(0,false).bp : arr1_shrink.bp ),
								   	arr1_shrink.bp + arr1_shrink.size, 
								    arr1_shrink.size};
	int res = pthread_mutex_init(&arr1_dynamic.mut, NULL);
	if(res != 0)
		printf("Mutex initialization failed in concat(...)\n");
	for(size_t i = 0; i < arr2_shrink.size; i++)
		push_back(&arr1_dynamic, arr2_shrink.bp[i]); // TODO memcpy if this is taking up too much time
	free(arr2_shrink.bp);
	return arr1_dynamic;
}
[[nodiscard]] dynamic_arr_info concat_unique(dynamic_arr_info * restrict arr1, dynamic_arr_info * restrict arr2){
	static_arr_info arr1_shrink = shrink_darr(arr1);
	static_arr_info arr2_shrink = shrink_darr(arr2);
	dynamic_arr_info arr1_dynamic = {((arr1_shrink.bp == NULL) ? init_darr(0,false).bp : arr1_shrink.bp ),
								   	arr1_shrink.bp + arr1_shrink.size, 
								    arr1_shrink.size};
	int res = pthread_mutex_init(&arr1_dynamic.mut, NULL);
	if(res != 0)
		printf("Mutex initialization failed in concat(...)\n");
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

void deduplicate(dynamic_arr_info *s){
    if(s->sp == s->bp || s-> sp == s->bp + 1){
		printf("Can't sort one value!\n");
		return;
	}
    dynamic_arr_info res = init_darr(0, 0.7 * (s->sp - s->bp)); // assume it's around 30% dupes
	qsort(s->bp, s->sp - s->bp, sizeof(uint64_t), compare);
	printf("Sorting: size: %lu\n", s->sp - s->bp);
	push_back(&res, *s->bp);
	for(uint64_t *curr = s->bp + 1; curr < s->sp; curr++){
		if(*curr != *(curr - 1)){
			push_back(&res, *curr);
		}
	}
	free(s->bp);
	*s = res;
	return;
}
