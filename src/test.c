#include "../inc/generate.h"
#include "../inc/solve.h"
#include "../inc/logging.h"
#include "../inc/board.h"
#include "../inc/settings.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool test_searching(void){
	const size_t test_size = 100;
	set_log_level(LOG_INFO_);
	log_out("Testing searching", LOG_INFO_);
	table t;
	t.key   = init_sarr(0, test_size); 
	t.value = init_sarr(0, test_size);
	for(size_t i = 0; i < test_size; i++){
		double val = (double)(test_size - i - 1);
		t.key.bp[i] = i;
		t.value.bp[i] = *(uint64_t*)(&val);
	}
	log_out("Test array:", LOG_TRACE_);
	for(size_t i = 0; i < t.key.size; i++){
		LOGIF(LOG_TRACE_){
			printf("Index: %ld, key: %ld, value: %lf\n", i, t.key.bp[i], *(double*)(&t.value.bp[i]));
		}
	}
	for(size_t i = 0; i < t.key.size; i++){
		LOGIF(LOG_TRACE_){
			printf("Looking for %ld (%ld) \n", i, (uint64_t)i);
		}
		if(lookup((uint64_t)i, &t, false) != (double)(test_size - i - 1)){
			log_out("Search test failed!", LOG_ERROR_);
			LOGIF(LOG_TRACE_){
				printf("Index: %ld, key: %ld, value: %lf\n", i, t.key.bp[i], lookup((uint64_t)i, &t, false));
			}
			free(t.key.bp);
			free(t.value.bp);
			return false;
		}
	}
	log_out("Search test passed", LOG_INFO_);
	free(t.key.bp);
	free(t.value.bp);
	return true;
}

bool test_dynamic_arr(void){
	dynamic_arr_info tmp = {0};
	log_out("Testing initialization (n = [0,50])\n", LOG_INFO_);
	for(size_t i = 0; i < 50; i++){
		tmp = init_darr(false, i);
		for(size_t j = 0; j < i; j++){
			tmp.bp[j] = 0; // test that we do really have access to all elements
		}
		destroy_darr(&tmp);
	}
	log_out("Testing resizing (n = [0,50])\n", LOG_INFO_);
	for(size_t i = 0; i < 50; i++){
		tmp = init_darr(false, 0);
		for(size_t j = 0; j < i; j++){
			push_back(&tmp, 0);
			if((size_t)(tmp.sp - tmp.bp) > tmp.size){
				log_out("Resizing failed!", LOG_ERROR_);
			}
		}
		destroy_darr(&tmp);
	}
	log_out("Testing concatenation (n,m = [0,50])\n", LOG_INFO_);
	for(size_t n = 0; n < 50; n++){
		for(size_t m = 0; m < 50; m++){
			dynamic_arr_info tmpn = init_darr(false, n);
			dynamic_arr_info tmpm = init_darr(false, m);
			LOGIF(LOG_TRACE_){
				printf("Testing n: %ld, m: %ld\n", n, m);
			}
			for(size_t i = 0; i < n; i++){
				push_back(&tmpn, i);
			}
			for(size_t i = 0; i < m; i++){
				push_back(&tmpm, i);
			}
			dynamic_arr_info nm = concat(&tmpn, &tmpm);
			LOGIF(LOG_TRACE_){
				printf("n.size = %ld, m.size = %ld, nm.size = %ld\n", tmpn.sp - tmpn.bp, tmpm.sp - tmpm.bp, nm.sp - nm.bp);
			}
			if((size_t)(nm.sp - nm.bp) != n + m){
				char *buf = malloc_errcheck(100);
				snprintf(buf, 100, "Concatenation test failed! n: %zu, m: %zu\n", n, m);
				log_out(buf, LOG_ERROR_);
				free(buf);
				return false;
			}
			for(size_t i = 0; i < n + m; i++){
				if(i < n){
				    if(nm.bp[i] != i){
						log_out("Concatenation test failed!\n", LOG_ERROR_);
						return false;
					}
				}
				else{
				    if(nm.bp[i] != i - n){
						log_out("Concatenation test failed!\n", LOG_ERROR_);
						return false;
					}
				}
			}
			free(nm.bp);
			nm.bp = NULL;
			nm.sp = NULL;
			nm.size = 0;
		}
	}
	log_out("No errors reported.\n", LOG_INFO_);
	return true;
}

void test_generation(void){
	log_out("Testing generation (correctness not checked).\n", LOG_INFO_);
	dynamic_arr_info n = init_darr(false, 0);
	push_back(&n, 0x1000002000000000); // board with a 2 and a 4 in a kinda arbitrary position
	generate(get_sum(n.bp[0]), get_sum(n.bp[0]) + 16, "/dev/null", n.bp, 1, 1, 0, 0, 0);
	log_out("Done testing generation.\n", LOG_INFO_);
}

bool test_dedupe(void){
	set_log_level(LOG_INFO_);
    log_out("Testing deduplication with artificial data.\n", LOG_INFO_);
	dynamic_arr_info d = init_darr(false, 0);
	push_back(&d, 2);
	push_back(&d, 5);
	push_back(&d, 6);
	push_back(&d, 5);
	push_back(&d, 4);
	push_back(&d, 5);
	for(int i = 1; i < 5; i++){
		deduplicate(&d);
		for(uint64_t *a = d.bp; a < d.sp; a++){
			for(uint64_t *b = d.bp; b < d.sp; b++){
				if(*a == *b && a != b){
					log_out("Failed!", LOG_ERROR_);
					set_log_level(LOG_INFO_);
					return false;
				}
			}
		}
	}
	log_out("No error reported.", LOG_INFO_);
	set_log_level(LOG_INFO_);
	destroy_darr(&d);
	return true;
}

bool test_rots(void){
	log_out("Testing symmetry generation", LOG_INFO_);
	uint64_t board;
	uint64_t *rots;
	int sum;
	const size_t iterations = 10000;
	// test a bunch of "random values"
	for(size_t i = 0; i < iterations; i++){
		board = rand();
		sum = get_sum(board);
		rots = get_all_rots(board);
		for(int rot = 0; rot < 8; rot++){
			if(get_sum(rots[rot]) != sum){
				char *buf = malloc_errcheck(100);
				snprintf(buf, 100, "%016lx is not a symmetry of %016lx!\n", rots[rot], board);
				log_out(buf, LOG_ERROR_);
				log_out("Symmetry test failed!", LOG_ERROR_);
				LOGIF(LOG_TRACE_){
					log_out("Other boards:", LOG_TRACE_);
					for(int j = 0; j < 8; j++)
						printf("[TRACE] %d : %016lx\n", j, rots[j]);
				}
				free(buf);
				return false;
			}
			for(int j = 0; j < 8; j++){
				if(rots[rot] == rots[j] && j != rot){
					char *buf = malloc_errcheck(100);
					snprintf(buf, 100, "%016lx is not a unique symmetry of %016lx! (%d, %d)\n", rots[rot], board, j, rot);
					log_out(buf, LOG_ERROR_);
					log_out("Symmetry test failed!", LOG_ERROR_);
					free(buf);
					return false;
				}
			}
		}
		free(rots);
	}
	log_out("Testing canonicalization", LOG_INFO_);
	for(size_t i = 0; i < iterations; i++){
		board = rand();
		rots = get_all_rots(board);
		for(int rot = 0; rot < 8; rot++)
			canonicalize_b(rots + rot);
		for(int a = 0; a < 8; a++){
			for(int b = 0; b < 8; b++){
				if(rots[a] != rots[b]){
					log_out("Failed!", LOG_ERROR_);
					return false;
				}
			}
		}
		free(rots);
	}
	log_out("No errors reported.", LOG_INFO_);
	return true;
}

bool test_misc(void){
	bool res = true;
	log_out("Testing tile detection", LOG_INFO_);
	const size_t iterations = 10000;
	for(char x = 0; x <= 0xf; x++){
		for(size_t i = 0; i < iterations; i++){
			bool flag = false;
			uint64_t board = rand();
			for(int i = 0; i < 16; i++){
				if((GET_TILE(board, i)) == x)
					flag = true;
			}
			if(!flag != checkx(board, x)){
				log_out("Failed!", LOG_ERROR_);
				res = false;
				LOGIF(LOG_DBG_){
					printf("-> Board: %016lx\n-> X: %d\n", board, x);
				}
			}
		}
	}
	log_out("Testing masking and unmasking with one board", LOG_INFO_);
	uint64_t board = 0x167f26ff54ff2231;
	log_out("Masked board:", LOG_INFO_);
	output_board(mask_board(board, get_settings().smallest_large));
	dynamic_arr_info tmp = unmask_board(mask_board(board, get_settings().smallest_large), get_settings().smallest_large, get_sum(board));
	log_out("Unmasked boards:", LOG_INFO_);
	for(uint64_t *curr = tmp.bp; curr < tmp.sp; curr++){
		output_board(*curr);
		if(curr + 1 != tmp.sp)
			log_out("---", LOG_INFO_);
	}
	log_out("Testing masking and unmasking with random boards", LOG_INFO_);
	destroy_darr(&tmp);
	for(size_t i = 0; i < iterations; i++){
		uint64_t old;
		board = rand();
		old = board;
		if(!checkx(board, 0xe))
			continue;
		board = mask_board(board, 6);
		tmp = unmask_board(board, 6, get_sum(old));
		bool found = false;
		for(uint64_t *curr = tmp.bp; curr < tmp.sp; curr++){
			if(*curr == old)
				found = true;
		}
		if(!found){
			log_out("Failed!", LOG_ERROR_);
			log_out("Original board", LOG_ERROR_);
			output_board(old);
			log_out("Masked board", LOG_ERROR_);
			output_board(board);
			log_out("Unmasked boards", LOG_ERROR_);
			for(uint64_t *curr = tmp.bp; curr < tmp.sp; curr++){
				printf("----");
				output_board(*curr);
			}
			destroy_darr(&tmp);
			return false;
		}
		destroy_darr(&tmp);
	}
	log_out("Testing bit operations", LOG_INFO_);
#define SETBIT(x, y) (x |= (1 << y))
#define GETBIT(x, y) (x & (1 << y))
	uint16_t test = 0;
	for(int i = 0; i < 16; i++){
		if(GETBIT(test, i)){
			log_out("Wrong!", LOG_ERROR_);
			return false;
		}
	}
	for(int i = 0; i < 16; i++){
		SETBIT(test, i);
		for(int j = 0; j <= i; j++){
			if(!GETBIT(test, j)){
				log_out("Wrong!", LOG_ERROR_);
				return false;
			}
		}
		for(int j = i + 1; j < 16; j++){
			if(GETBIT(test, j)){
				log_out("Wrong!", LOG_ERROR_);
				return false;
			}
		}
	}
	for(int i = 0; i < 16; i++){
		if(!GETBIT(test, i)){
			log_out("Wrong!", LOG_ERROR_);
			return false;
		}
	}
#undef SETBIT
#undef GETBIT
	log_out("No errors reported", LOG_INFO_);
	return res;
}

static bool test_settings(void){
	log_out("Checking settings (accuracy not checked, verify manually)", LOG_INFO_);
	settings_t settings = get_settings();
	printf(".free_formation %d\n", settings.free_formation);
	printf(".cores %lld\n", settings.cores);
	printf(".nox %lld\n", settings.nox);
	printf(".mask %d\n", settings.mask);
	printf(".premove %d\n", settings.premove);
	printf(".bdir %s\n", settings.bdir);
	printf(".initial %s\n", settings.initial);
	printf(".end_gen %lld\n", settings.end_gen);
	printf(".stsl %lld\n", settings.stsl);
	printf(".smallest_large %lld\n", settings.smallest_large);
	printf(".prune %d\n", settings.prune);
	printf(".tdir %s\n", settings.tdir);
	printf(".winstates %s\n", settings.winstates);
	printf(".end_solve %lld\n", settings.end_solve);
	printf(".score %d\n", settings.score);
	return true;
}

bool test(void){
	set_log_level(LOG_INFO_);
	bool passed = true;
	generate_lut();
	passed &= test_dynamic_arr();
	passed &= test_searching();
	passed &= test_dedupe();
	passed &= test_rots();
	passed &= test_misc();
	passed &= test_settings();
	test_generation();
	if(!passed){
		log_out("One or more tests failed!", LOG_ERROR_);
	}
	else
		log_out("All tests passed!", LOG_INFO_);
	return passed;
}
