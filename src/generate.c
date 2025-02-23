#include "../inc/generate.h"
#include "../inc/main.h"
#include "../inc/board.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
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

static void write_boards(const static_arr_info n, const char* fmt, const int layer){
	size_t filename_size = strlen(fmt) + 10; // if there are more than 10 digits of layers i'll eat my shoe
	char* filename = malloc(sizeof(char) * filename_size);
	snprintf(filename, filename_size, fmt, layer);
	printf("Writing %lu boards to %s (%lu bytes)\n", n.size, filename, sizeof(uint64_t) * n.size); 
	FILE *file = fopen(filename, "wb");
	if(file == NULL){
		printf("Couldn't write to %s!\n", filename);
		return;
	}
	fwrite(n.bp, n.size, sizeof(uint64_t), file);
	fclose(file);
}

void* generation_thread_move(void* data){
	arguments *args = data;
	uint64_t tmp;
	bool (*move)(uint64_t*, dir) = flat_movement ? flat_move : movedir;
	int res = pthread_mutex_trylock(args->done);
	if (res != 0){ // the caller has been naughty
		printf("Worker thread mutex (%lu) couldn't be locked, check caller logic!(%d)\n", (unsigned long)args->done, res);
	}
	for(size_t i = args->start; i < args->end; i++){
		tmp = args->n.bp[i];
		for(dir d = left; d < down; d++){
			if(move(&tmp, d)){
				//canonicalize(&tmp);
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
		printf("Worker thread mutex (%lu) couldn't be locked, check caller logic!(%d)\n", (unsigned long)args->done, res);
	}
	for(size_t i = args->start; i < args->end; i++){
		for(int tile = 0; tile < 16; tile++){
			if(GET_TILE((args->n).bp[i], tile) == 0){
				tmp = args->n.bp[i];
				SET_TILE(tmp, tile, 1);
				//canonicalize(&tmp);
				push_back(args->n2, tmp);
				tmp = args->n.bp[i];
				bool dupe = spawn_duplicate(tmp); // we have to check here; also only checking four spawn because two spawn can't be a dupe
				SET_TILE(tmp, tile, 2);
				//canonicalize(&tmp);
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
			else
				pthread_mutex_unlock(&cores[i].done); // we own this mutex so we have to unlock it before we try to lock it again
		}
	}
	// if we're here, all the cores are done
	for(uint i = 0; i < core_count; i++){
		pthread_join(cores[i].core, NULL); // idc abt the return
	}
}

void generate_layer(dynamic_arr_info * restrict n, dynamic_arr_info * restrict n2, dynamic_arr_info * restrict n4, 
		dynamic_arr_info * restrict potential_duplicate, const uint core_count, const char *fmt_dir, const int layer){ // this is completely nonsensical if any of the pointers are the same, hence restrict
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
	if(potential_duplicate->size > 0){
		*n = concat_unique(n, potential_duplicate);
		*potential_duplicate = init_darr(0,1);
	}
	init_threads(cores, args, *n, potential_duplicate, core_count, arr_size_per_thread, true, generation_thread_spawn);
	// write while we're waiting for the spawning threads
	write_boards((static_arr_info){n->bp, n->sp - n->bp}, fmt_dir, layer);
	wait_all(cores, core_count);
	for(uint i = 0; i < core_count; i++){ 
		*n2 = concat(n2, &cores[i].n2);
		*n4 = concat(n4, &cores[i].n4);
	}
	*n4 = concat_unique(n4, potential_duplicate); // only n4 will have the dupes
	*potential_duplicate = init_darr(0,1);
}
void generate(const int start, const int end, const char* fmt, uint64_t* initial, const size_t initial_len, const uint core_count, bool prespawn){
	dynamic_arr_info n, n2, n4, potential_duplicate;
	n.bp = initial;
	n.sp = n.bp + initial_len;
	n.size = initial_len;
	// initialize arrays
	n2 = init_darr(false, 0);
	n4 = init_darr(false, 0);
	potential_duplicate = init_darr(false, 0);
	if(prespawn){ // spawn once before moving
		printf("Prespawning...\n");
		for(uint64_t *curr = n.bp; curr < n.sp; curr++){
			for(int i = 0; i < 16; i++){
				if(GET_TILE((*curr), i) == 0){
					SET_TILE((*curr), i, 1);
					push_back(&n2, (*curr));
					SET_TILE((*curr), i, 2);
					push_back(&n4, (*curr));
					SET_TILE((*curr), i, 0);
				}
			}
		}
	}
	printf("Generating...\n");
	printf("Starting boards(%d): \n", start);
	for(int i = 0; i < initial_len; i++)
		printf("0x%016lx\n",n.bp[i]);
	printf("Starting boards(%d): \n", start+2);
	for(int i = 0; i < n2.sp - n2.bp; i++)
		printf("0x%016lx\n",n2.bp[i]);
	printf("Starting boards(%d): \n", start+4);
	for(int i = 0; i < n4.sp - n4.bp; i++)
		printf("0x%016lx\n",n4.bp[i]);
	for(int i = start; i < end; i += 2){
		generate_layer(&n, &n2, &n4, &potential_duplicate, core_count, fmt, i);
		free(n.bp);
		n = n2;
		n2 = n4;
		n4 = init_darr(false, 0);
	}
}

static_arr_info read_table(const char *dir){
	FILE *fp = fopen(dir, "rb");
	fseek(fp, 0L, SEEK_END);
	size_t sz = ftell(fp);
	rewind(fp);
	if(sz % 8 != 0)
		printf("sz %%8 != 0, this is probably not a real table!\n");
	uint64_t* data = malloc(sz);
	if(data == NULL)
		printf("Could not allocate space to read table!\n");
	fread(data, 1, sz, fp);
	printf("Read %ld bytes (%ld boards) from %s\n", sz, sz / 8, dir);
	fclose(fp);
	return (static_arr_info){data, sz / 8}; 
}
