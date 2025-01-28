#include "../inc/generate.h"
#include "../inc/board.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#define PREPRUNE 1
int init_errorcheck_mutex(pthread_mutex_t *mutex)
{
    pthread_mutexattr_t attr;
    int r;

    r = pthread_mutexattr_init(&attr);
    if (r != 0)
        return r;

    r = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);

    if (r == 0)
        r = pthread_mutex_init(mutex, &attr);

    pthread_mutexattr_destroy(&attr);

    return r;
}
struct {
	static_arr_info n; 
	dynamic_arr_info* nret;
	dynamic_arr_info* n2; 
	dynamic_arr_info* n4;
	pthread_mutex_t* done; 
	size_t start; 
	size_t end; 
	dynamic_arr_info *potential_duplicate;
} typedef arguments;

struct {
	pthread_t core;
	dynamic_arr_info nret;
	dynamic_arr_info n2;
	dynamic_arr_info n4;
	pthread_mutex_t done;
}typedef core_data;

void* generation_thread_move(void* data){
	arguments *args = data;
	uint64_t tmp;
	int res = pthread_mutex_trylock(args->done);
	if (res != 0){ // the caller has been naughty
		printf("Worker thread mutex couldn't be locked, check caller logic!\n");
		exit(res);
	}
	for(size_t i = args->start; i < args->end; i++){
		tmp = args->n.bp[i];
		for(dir d = left; d < down; d++){
			if(movedir(&tmp, d)){
				canonicalize(&tmp);
				push_back(args->nret, tmp);
			}
		}
	}
	pthread_mutex_unlock(args->done);
	return NULL;
}
void* generation_thread_spawn(void* data){
	arguments *args = data;
	int res = pthread_mutex_trylock(args->done);
	uint64_t tmp;
	if (res != 0){ // the caller has been naughty
		printf("Worker thread mutex couldn't be locked, check caller logic!\n");
		exit(res);
	}
	for(size_t i = args->start; i < args->end; i++){
		for(int tile = 0; tile < 16; tile++){
			if(GET_TILE((args->n).bp[i], tile) == 0){
				tmp = args->n.bp[i];
				SET_TILE(tmp, tile, 1);
				canonicalize(&tmp);
				push_back(args->n2, tmp);
				tmp = args->n.bp[i];
				bool dupe = spawn_duplicate(tmp); // we have to check here; also only checking four spawn because two spawn can't be a dupe
				SET_TILE(tmp, tile, 2);
				canonicalize(&tmp);
				if(!dupe)
					push_back(args->n4, tmp);
				else
					push_back(args->potential_duplicate, tmp);
			}
		}
	}
	pthread_mutex_unlock(args->done);
	return NULL;
}

void init_threads(core_data* cores, arguments* args, dynamic_arr_info n, dynamic_arr_info *potential_duplicate,
		size_t core_count, size_t arr_size_per_thread, bool spawn, void* (worker_thread)(void*)){
	for(uint i = 0; i < core_count; i++){ // initialize worker threads
		if(spawn){
			cores[i].n2 = init_darr(false, arr_size_per_thread / 2);
			cores[i].n4 = init_darr(false, arr_size_per_thread / 2);
		}
		else{
			cores[i].nret = init_darr(false, arr_size_per_thread);
		}

		args[i].n = (static_arr_info){n.bp, n.sp - n.bp};
		args[i].nret = &cores[i].nret;
		args[i].n2 = &cores[i].n2;
		args[i].n4 = &cores[i].n4;
		args[i].done = &cores[i].done;
		args[i].potential_duplicate = potential_duplicate;
		// divide up [0,n.size)
		// cores work in [start,end)
		int block_size = n.size / core_count;
		args[i].start = i * block_size;
		args[i].end = (i + 1) * block_size;
		// core_count * n.size / core_count = n.size
		// make sure that the last thread covers all of the array
		if(i + 1 == core_count){
			args[i].end = args[i].n.size;
		}
		int res = pthread_create(&cores[i].core, NULL, worker_thread, (void*)(args + i));
		if(res != 0){
			printf("Core initialization failed: Error %d, Core %d", res, i);
			exit(res);
		}
	}
}

void wait_all(core_data *cores, size_t core_count){
	static_arr_info done_arr = init_sarr(false, core_count);
	bool done = false;
	while(done == false){
		done = true;
		for(uint i = 0; i < core_count; i++){
			done_arr.bp[i] = pthread_mutex_trylock(&cores[i].done);
			if (done_arr.bp[i] != 0)
				done = false;
		}
		for(uint i = 0; i < core_count; i++){
			if(done_arr.bp[i] == 0)
				pthread_mutex_unlock(&cores[i].done); // we own this mutex so we have to unlock it before we try to lock it
		}
	}
	// if we're here, all the cores are done
	for(uint i = 0; i < core_count; i++){
		pthread_mutex_unlock(&cores[i].done); // i don't even care anymore man
		pthread_join(cores[i].core, NULL); // idc abt the return
	}
}

void generate_layer(const char* file, dynamic_arr_info * restrict n, dynamic_arr_info * restrict n2, 
		dynamic_arr_info * restrict n4, dynamic_arr_info * restrict potential_duplicate, const uint core_count){ // this is completely nonsensical if any of the pointers are the same, hence restrict
	// make an initial estimate for the amount of spawns per tile: TODO test efficacy of different values
	size_t approx_len = 24 * n->size; // 6 open spaces and 4 directions: definitely an overestimate
	size_t arr_size_per_thread = approx_len / core_count;
	static bool core_init = false;
	static core_data* cores;
	static arguments* args;

	if(!core_init){
		cores = malloc(sizeof(core_data) * core_count); // make an array of core data
		if(cores == NULL){
			printf("Alloc failed!\n");
			exit(1);
		}
		for(size_t i = 0; i < core_count; i++){
			int res = init_errorcheck_mutex(&cores[i].done);
			if(res != 0){
				printf("Failed to init core mutex: errcode %d", res);
				exit(res);
			}
		}
		args = malloc(sizeof(arguments) * core_count); // args
		if(args == NULL){
			printf("Alloc failed!\n");
			exit(1);
		}
		core_init = true;
	}
	init_threads(cores, args, *n, potential_duplicate, core_count, arr_size_per_thread, false, generation_thread_move);
	// twiddle our thumbs
	// TODO: there's some work we can do by concating all the results as they're coming in instead of waiting for all of them
	wait_all(cores, core_count);
	// concatenate all the data TODO: maybe there is some clever way to do this?
	for(uint i = 0; i < core_count; i++){ 
		*n = concat(n, &cores[i].nret);
	}
	// deal with the potential dupes, very slowly
	*n = concat_unique(n, potential_duplicate);
	init_threads(cores, args, *n, potential_duplicate, core_count, arr_size_per_thread, true, generation_thread_spawn);
	wait_all(cores, core_count);
	for(uint i = 0; i < core_count; i++){ 
		*n2 = concat(n2, &cores[i].n2);
		*n4 = concat(n4, &cores[i].n4);
	}
	*n4 = concat_unique(n4, potential_duplicate); // only n4 will have the dupes
}
void generate(const int start, const int end, const char* fmt, uint64_t* initial, const size_t initial_len, const uint core_count){
	char buf[256];
	buf[255] = '\0';
	dynamic_arr_info n, n2, n4, potential_duplicate;
	n.bp = initial;
	n.sp = n.bp;
	n.size = initial_len;
	// initialize arrays
	n2 = init_darr(false, 0);
	n4 = init_darr(false, 0);
	potential_duplicate = init_darr(false, 0);
	for(int i = start; i < end; i++){
		memset(buf,0,strlen(buf)); // reset the buffer
		buf[255] = '\0';
		snprintf(buf, strlen(buf), fmt, i); // make the file name
		generate_layer(buf, &n, &n2, &n4, &potential_duplicate, core_count);
	}
}
