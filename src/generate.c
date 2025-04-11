#include "../inc/generate.h"
#include "../inc/logging.h"
#include "../inc/board.h"
#include "../inc/main.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
struct {
	pthread_t core;
	static_arr_info n; 
	dynamic_arr_info* nret;
	dynamic_arr_info* n2; 
	dynamic_arr_info* n4;
	size_t start; 
	size_t end; 
	dynamic_arr_info *potential_duplicate;
} typedef arguments;
static void write_boards(const static_arr_info n, const char* fmt, const int layer){
	size_t filename_size = strlen(fmt) + 10; // if there are more than 10 digits of layers i'll eat my shoe
	char* filename = malloc_errcheck(sizeof(char) * filename_size);
	snprintf(filename, filename_size, fmt, layer);
	printf("[INFO] Writing %lu boards to %s (%lu bytes)\n", n.size, filename, sizeof(uint64_t) * n.size);  // lol
	FILE *file = fopen(filename, "wb");
	if(file == NULL){
		char *buf = malloc_errcheck(100);
		snprintf(buf, 100, "Couldn't write to %s!\n", filename);
		log_out(buf, LOG_WARN_);
		free(buf);
		return;
	}
	fwrite(n.bp, n.size, sizeof(uint64_t), file);
	fclose(file);
}

bool shifted(uint64_t a, uint64_t b){
    for(int i = 0; i < 16; i++){
    	if(GET_TILE(a, i) == 0xF && GET_TILE(b, i) != 0xF){
		    return true;      		
    	}
    	if(GET_TILE(b, i) == 0xF && GET_TILE(a, i) != 0xF){
		    return true;      		
    	}
    }
	return false;
}

spawn_ret spawnarr(const static_arr_info, dynamic_arr_info*){
	
}
uint64_t* movearr(const static_arr_info, dynamic_arr_info*){

}
void generate_layer(dynamic_arr_info* n, dynamic_arr_info* n2, dynamic_arr_info* n4, 
		const uint, const char *fmt_dir, const int layer){

}
void generate(const int, const int, const char*, uint64_t*, const size_t, const uint, bool prespawn){

}
static_arr_info read_boards(const char *dir){
	FILE *fp = fopen(dir, "rb");
	fseek(fp, 0L, SEEK_END);
	size_t sz = ftell(fp);
	rewind(fp);
	if(sz % 8 != 0)
		log_out("sz %%8 != 0, this is probably not a real table!\n", LOG_WARN_);
	uint64_t* data = malloc_errcheck(sz);
	fread(data, 1, sz, fp);
	char *buf = malloc_errcheck(100);
	snprintf(buf, 100, "Read %ld bytes (%ld boards) from %s\n", sz, sz / 8, dir);
	log_out(buf, LOG_DBG_);
	free(buf);
	fclose(fp);
	return (static_arr_info){true, data, sz / 8}; 
}

