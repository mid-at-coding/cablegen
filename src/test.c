#include "../inc/generate.h"
#include "../inc/logging.h"
#include "../inc/board.h"
#include "../inc/main.h"
#include <stdio.h>

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
			if(nm.sp - nm.bp != n + m){
				char *buf = malloc(100);
				snprintf(buf, 100, "Concatenation test failed! n: %zu, m: %zu\n", n, m);
				log_out(buf, LOG_ERROR_);
				free(buf);
				passed = false;
			}
			for(int i = 0; i < n + m; i++){
				if(i < n){
				    if(nm.bp[i] != i){
						log_out("Concatenation test failed!\n", LOG_ERROR_);
						passed = false;
					}
				}
				else{
				    if(nm.bp[i] != i - n){
						log_out("Concatenation test failed!\n", LOG_ERROR_);
						passed = false;
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
	log_out("Generation test temporarily disabled", LOG_WARN_);
/*	log_out("Testing generation (correctness not checked).\n", LOG_INFO_);
	dynamic_arr_info n = init_darr(false, 0);
	push_back(&n, 0x1000002000000000); // board with a 2 and a 4 in a kinda arbitrary position
	dynamic_arr_info n2 = init_darr(false, 0);
	dynamic_arr_info n4 = init_darr(false, 0);
	dynamic_arr_info pd = init_darr(false, 0);
	generate_layer(&n, &n2, &n4, &pd, 1, "/dev/null/%d", 6);
	log_out("Done testing generation.\n", LOG_INFO_); */
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
	log_out("Testing deduplication in generation.", LOG_INFO_);
	dynamic_arr_info n = init_darr(false, 0);
	push_back(&n, 0x1000002000000000); // board with a 2 and a 4 in a kinda arbitrary position
	dynamic_arr_info n2 = init_darr(false, 0);
	dynamic_arr_info n4 = init_darr(false, 0);
	dynamic_arr_info pd = init_darr(false, 0);
//	log_out("Testing sorting deduplication.\n", LOG_INFO_);
	log_out("Sorting deduplication test temporarily disabled.", LOG_WARN_);
/*	for(int i = 6; i < 12; i+=2){
		generate_layer(&n, &n2, &n4, &pd, 1, "/dev/null/%d", i);
		// check if n is dupe free
		deduplicate(&n);
		for(uint64_t *a = n.bp; a < n.sp; a++){
			for(uint64_t *b = n.bp; b < n.sp; b++){
				if(*a == *b && a != b){
					char *buf = malloc_errcheck(100);
				    snprintf(buf, 100, "Failed!(%lu, %lu)\n", a, b);
					log_out(buf, LOG_ERROR_);
					free(buf);
					return false;
				}
			}
		}
		free(n.bp);
		n = n2;
		n2 = n4;
		n4 = init_darr(false, 0);
	} */
	log_out("No errors reported.", LOG_INFO_);
	return true;
}

bool test(){
	bool passed = true;
	generate_lut(true);
	if(!test_dynamic_arr())
		passed = false;
	if(!test_dedupe())
		passed = false;
	test_generation();
	return passed;
}
