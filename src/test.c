#include "../inc/generate.h"
#include "../inc/solve.h"
#include "../inc/logging.h"
#include "../inc/board.h"
#include "../inc/main.h"
#include <stdio.h>

bool test_searching(void){
	set_log_level(LOG_DBG_);
	const size_t test_size = 100;
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
			return false;
		}
	}
	log_out("Search test passed", LOG_INFO_);
	return true;
}

bool test_dynamic_arr(void){
	bool passed = true;
	dynamic_arr_info tmp;
	log_out("Testing initialization (n = [0,50])\n", LOG_INFO_);
	for(size_t i = 0; i < 50; i++){
		tmp = init_darr(false, i);
		for(size_t j = 0; j < i; j++){
			tmp.bp[j] = 0; // test that we do really have access to all elements
		}
		free(tmp.bp);
	}
	log_out("Testing resizing (n = [0,50])\n", LOG_INFO_);
	for(size_t i = 0; i < 50; i++){
		tmp = init_darr(false, 0);
		for(size_t j = 0; j < i; j++){
			push_back(&tmp, 0);
		}
		free(tmp.bp);
	}
	log_out("Testing concatenation (n,m = [0,50])\n", LOG_INFO_);
	for(size_t n = 0; n < 50; n++){
		for(size_t m = 0; m < 50; m++){
			dynamic_arr_info tmpn = init_darr(false, n);
			dynamic_arr_info tmpm = init_darr(false, m);
			for(size_t i = 0; i < n; i++){
				push_back(&tmpn, i);
			}
			for(size_t i = 0; i < m; i++){
				push_back(&tmpm, i);
			}
			dynamic_arr_info nm = concat(&tmpn, &tmpm);
			if((size_t)(nm.sp - nm.bp) != n + m){
				char *buf = malloc(100);
				snprintf(buf, 100, "Concatenation test failed! n: %zu, m: %zu\n", n, m);
				log_out(buf, LOG_ERROR_);
				free(buf);
				passed = false;
				return false;
			}
			for(size_t i = 0; i < n + m; i++){
				if(i < n){
				    if(nm.bp[i] != i){
						log_out("Concatenation test failed!\n", LOG_ERROR_);
						passed = false;
						return false;
					}
				}
				else{
				    if(nm.bp[i] != i - n){
						log_out("Concatenation test failed!\n", LOG_ERROR_);
						passed = false;
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
	if(passed){
		log_out("No errors reported.\n", LOG_INFO_);
	}
	return passed;
}

void test_generation(){
	log_out("Testing generation (correctness not checked).\n", LOG_INFO_);
	dynamic_arr_info n = init_darr(false, 0);
	push_back(&n, 0x1000002000000000); // board with a 2 and a 4 in a kinda arbitrary position
	generate(get_sum(n.bp[0]), get_sum(n.bp[0]) + 16, "/dev/null", n.bp, 1, 1, 0);
	log_out("Done testing generation.\n", LOG_INFO_);
}

bool test_dedupe(){
    log_out("Testing deduplication with artificial data.\n", LOG_INFO_);
	dynamic_arr_info d = init_darr(false, 0);
	push_back(&d, 2);
	push_back(&d, 5);
	push_back(&d, 6);
	push_back(&d, 5);
	push_back(&d, 4);
	push_back(&d, 5);
	deduplicate(&d);
	for(uint64_t *a = d.bp; a < d.sp; a++){
		for(uint64_t *b = d.bp; b < d.sp; b++){
			if(*a == *b && a != b){
			    log_out("Failed!", LOG_ERROR_);
				return false;
			}
		}
	}
	log_out("No error reported.", LOG_INFO_);
	return true;
}

bool test_rots(void){
	set_log_level(LOG_TRACE_);
	log_out("Testing symmetry generation", LOG_INFO_);
	uint64_t board;
	uint64_t *rots;
	int sum;
	const size_t iterations = 100;
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
				return false;
			}
		}
	}
	log_out("No errors reported.", LOG_INFO_);
	return true;
}

bool test(){
	bool passed = true;
	generate_lut(true);
	passed &= test_dynamic_arr();
	passed &= test_searching();
	passed &= test_dedupe();
	passed &= test_rots();
	test_generation();
	return passed;
}
