#include "array.h"
#include "generate.h"
#include "solve.h"
#define LOG_H_ENUM_PREFIX_
#define LOG_H_NAMESPACE_ 
#include "logging.h"
#include "board.h"
#include "settings.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool test_searching(void){
	const size_t test_size = 100;
	set_log_level(LOG_INFO);
	log_out("Testing searching", LOG_INFO);
	table t;
	t.key   = init_sarr(0, test_size); 
	t.value = init_sarr(0, test_size);
	for(size_t i = 0; i < test_size; i++){
		double val = (double)(test_size - i - 1);
		t.key.bp[i] = i;
		t.value.bp[i] = *(uint64_t*)(&val);
	}
	log_out("Test array:", LOG_TRACE);
	for(size_t i = 0; i < t.key.size; i++){
		logf_out("Index: %ld, key: %ld, value: %lf", LOG_TRACE, i, t.key.bp[i], *(double*)(&t.value.bp[i]));
	}
	for(size_t i = 0; i < t.key.size; i++){
		logf_out("Looking for %ld (%ld) ", LOG_TRACE, i, (uint64_t)i);
		if(lookup((uint64_t)i, &t, false) != (double)(test_size - i - 1)){
			log_out("Search test failed!", LOG_ERROR);
			logf_out("Index: %ld, key: %ld, value: %lf", LOG_ERROR, i, t.key.bp[i], lookup((uint64_t)i, &t, false));
			free(t.key.bp);
			free(t.value.bp);
			return false;
		}
	}
	log_out("Search test passed", LOG_INFO);
	free(t.key.bp);
	free(t.value.bp);
	return true;
}

bool test_dynamic_arr(void){
	dynamic_arr_info tmp = {0};
	log_out("Testing initialization (n = [0,50])", LOG_INFO);
	for(size_t i = 0; i < 50; i++){
		tmp = init_darr(false, i);
		for(size_t j = 0; j < i; j++){
			tmp.bp[j] = 0; // test that we do really have access to all elements
		}
		destroy_darr(&tmp);
	}
	log_out("Testing resizing (n = [0,50])", LOG_INFO);
	for(size_t i = 0; i < 50; i++){
		tmp = init_darr(false, 0);
		for(size_t j = 0; j < i; j++){
			push_back(&tmp, 0);
			if((size_t)(tmp.sp - tmp.bp) > tmp.size){
				log_out("Resizing failed!", LOG_ERROR);
			}
		}
		destroy_darr(&tmp);
	}
	log_out("Testing concatenation (n,m = [0,50])", LOG_INFO);
	for(size_t n = 0; n < 50; n++){
		for(size_t m = 0; m < 50; m++){
			dynamic_arr_info tmpn = init_darr(false, n);
			dynamic_arr_info tmpm = init_darr(false, m);
			logf_out("Testing n: %ld, m: %ld", LOG_TRACE, n, m);
			for(size_t i = 0; i < n; i++){
				push_back(&tmpn, i);
			}
			for(size_t i = 0; i < m; i++){
				push_back(&tmpm, i);
			}
			dynamic_arr_info nm = concat(&tmpn, &tmpm);
			logf_out("n.size = %ld, m.size = %ld, nm.size = %ld", LOG_TRACE, tmpn.sp - tmpn.bp, tmpm.sp - tmpm.bp, nm.sp - nm.bp);
			if((size_t)(nm.sp - nm.bp) != n + m){
				logf_out("Concatenation test failed! n: %zu, m: %zu", LOG_ERROR, n, m);
				return false;
			}
			for(size_t i = 0; i < n + m; i++){
				if(i < n){
				    if(nm.bp[i] != i){
						log_out("Concatenation test failed!", LOG_ERROR);
						return false;
					}
				}
				else{
				    if(nm.bp[i] != i - n){
						log_out("Concatenation test failed!", LOG_ERROR);
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
	log_out("No errors reported.", LOG_INFO);
	log_out("Testing bucket insertion", LOG_INFO);
	buckets b;
	init_buckets(&b);
	for(size_t n = 0; n < 500; n++){
		uint64_t tmp = rand();
		push_back_into_bucket(&b, tmp);
		_BitInt(BUCKETS_DIGITS) lookup = get_first_digits(tmp);
		if(b.bucket[lookup].d.sp == b.bucket[lookup].d.bp){
			log_out("Bucket not pushed to!", LOG_ERROR);
			return false;
		}
		if(*(b.bucket[lookup].d.sp - 1) != tmp){
			log_out("Incorrect value!", LOG_ERROR);
			return false;
		}
	}
	destroy_buckets(&b);
	log_out("No errors reported.", LOG_INFO);
	return true;
}

void test_generation(void){
	log_out("Testing generation (correctness not checked).", LOG_INFO);
	dynamic_arr_info n = init_darr(false, 0);
	push_back(&n, 0x1000002000000000); // board with a 2 and a 4 in a kinda arbitrary position
	generate(get_sum(n.bp[0]), get_sum(n.bp[0]) + 16, "/dev/null", n.bp, 1, 1, 0, 0, 0);
	log_out("Done testing generation.", LOG_INFO);
}

bool test_dedupe(void){
	set_log_level(LOG_INFO);
	log_out("Testing deduplication with artificial data.", LOG_INFO);
	dynamic_arr_info d = init_darr(false, 0);
	push_back(&d, 2);
	push_back(&d, 5);
	push_back(&d, 6);
	push_back(&d, 5);
	push_back(&d, 4);
	push_back(&d, 5);
	deduplicate(&d);
	for(uint64_t *a = d.bp; a < d.sp; a++){
		logf_out("\t%p: %zu", LOG_INFO, a, *a);
		for(uint64_t *b = d.bp; b < d.sp; b++){
			if(*a == *b && a != b){
				log_out("Failed!", LOG_ERROR);
				set_log_level(LOG_INFO);
				return false;
			}
		}
	}
	log_out("Testing multi-array merging.", LOG_INFO);
	dynamic_arr_info d2 = init_darr(false, 0);
	push_back(&d2, 1);
	push_back(&d2, 5);
	push_back(&d2, 9);
	dynamic_arr_info darrs[2] = { d, d2 };
	dynamic_arr_info res = deduplicate_threads(darrs, 2);
	for(uint64_t *curr = res.bp; curr < res.sp; curr++){
		logf_out("\t%p: %zu", LOG_INFO, curr, *curr);
	}
	log_out("No error reported.", LOG_INFO);
	set_log_level(LOG_INFO);
	destroy_darr(&d);
	destroy_darr(&d2);
	destroy_darr(&res);
	return true;
}

bool test_rots(void){
	log_out("Testing symmetry generation", LOG_INFO);
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
				logf_out("%016lx is not a symmetry of %016lx!", LOG_ERROR, rots[rot], board);
				log_out("Symmetry test failed!", LOG_ERROR);
				log_out("Other boards:", LOG_TRACE);
				for(int j = 0; j < 8; j++)
					logf_out("%d : %016lx", LOG_TRACE, j, rots[j]);
				return false;
			}
			for(int j = 0; j < 8; j++){
				if(rots[rot] == rots[j] && j != rot){
					logf_out("%016lx is not a unique symmetry of %016lx! (%d, %d)", LOG_ERROR, rots[rot], board, j, rot);
					log_out("Symmetry test failed!", LOG_ERROR);
					return false;
				}
			}
		}
		free(rots);
	}
	log_out("Testing canonicalization", LOG_INFO);
	for(size_t i = 0; i < iterations; i++){
		board = rand();
		rots = get_all_rots(board);
		for(int rot = 0; rot < 8; rot++)
			canonicalize_b(rots + rot);
		for(int a = 0; a < 8; a++){
			for(int b = 0; b < 8; b++){
				if(rots[a] != rots[b]){
					log_out("Failed!", LOG_ERROR);
					return false;
				}
			}
		}
		free(rots);
	}
	log_out("No errors reported.", LOG_INFO);
	return true;
}

bool test_misc(void){
	bool res = true;
	log_out("Testing tile detection", LOG_INFO);
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
				log_out("Failed!", LOG_ERROR);
				res = false;
				logf_out("-> Board: %016lx\n-> X: %d", LOG_DBG, board, x);
			}
		}
	}
	log_out("Testing masking and unmasking with one board", LOG_INFO);
	log_out("Masking unit test temporarily disabled", LOG_WARN);
	/*
	uint64_t board = 0x168725ff54ff2231;
	set_log_level(LOG_TRACE);
	masked_board masked = mask_board(board, get_settings().smallest_large);
	set_log_level(LOG_INFO);
	masked_board_sorted masked2 = {masked.masked, init_darr(0, 0)};
	push_back(&masked2.unmasked, masked.unmasked);
	log_out("Unmasked board:", LOG_INFO);
	output_board(board);
	log_out("Masked board:", LOG_INFO);
	output_board(masked.masked);
	log_out("Masked tiles:", LOG_INFO);
	output_board(masked.unmasked);
	log_out("Unmasked boards:", LOG_INFO);
	dynamic_arr_info tmp = unmask_board(masked2, get_sum(board));
	for(uint64_t *i = tmp.bp; i < tmp.sp; i++){
		output_board(*i);
	} */
	log_out("Testing bit operations", LOG_INFO);
	uint16_t test = 0;
	for(int i = 0; i < 16; i++){
		if(GETBIT(test, i)){
			log_out("Wrong!", LOG_ERROR);
			return false;
		}
	}
	for(int i = 0; i < 16; i++){
		SETBIT(test, i);
		for(int j = 0; j <= i; j++){
			if(!GETBIT(test, j)){
				log_out("Wrong!", LOG_ERROR);
				return false;
			}
		}
		for(int j = i + 1; j < 16; j++){
			if(GETBIT(test, j)){
				log_out("Wrong!", LOG_ERROR);
				return false;
			}
		}
	}
	test = 0;
	for(int i = 0; i < 16; i++){
		CLEARBIT(test, i);
		if(GETBIT(test, i)){
			log_out("Wrong!", LOG_ERROR);
			return false;
		}
	}
	log_out("No errors reported", LOG_INFO);
	return res;
}

static bool test_settings(void){
	log_out("Checking settings (accuracy not checked, verify manually)", LOG_INFO);
	settings_t settings = *get_settings();
	logf_out(".free_formation %d", LOG_INFO, settings.free_formation);
	logf_out(".cores %lld", LOG_INFO, settings.cores);
	logf_out(".nox %lld", LOG_INFO, settings.nox);
	logf_out(".mask %d", LOG_INFO, settings.mask);
	logf_out(".premove %d", LOG_INFO, settings.premove);
	logf_out(".bdir %s", LOG_INFO, settings.bdir);
	logf_out(".initial %s", LOG_INFO, settings.initial);
	logf_out(".end_gen %lld", LOG_INFO, settings.end_gen);
	logf_out(".stsl %lld", LOG_INFO, settings.stsl);
	logf_out(".smallest_large %lld", LOG_INFO, settings.smallest_large);
	logf_out(".prune %d", LOG_INFO, settings.prune);
	logf_out(".tdir %s", LOG_INFO, settings.tdir);
	logf_out(".winstates %s", LOG_INFO, settings.winstates);
	logf_out(".end_solve %lld", LOG_INFO, settings.end_solve);
	logf_out(".score %d", LOG_INFO, settings.score);
	return true;
}

static bool test_cmpbrd(){
	log_out("Testing comparison", LOG_INFO);
	uint64_t test = 0;
	uint64_t curr = 0;
	uint64_t ind = 0;
	size_t iterations = 10000;
	for(size_t i = 0; i < iterations; i++){
		curr = rand();
		if(!cmpbrd(curr, test)){
			log_out("Wrong!", LOG_ERROR);
			output_board(curr);
			log_out("Not equal", LOG_ERROR);
			output_board(test);
			return false;
		}
		// choose a random tile to set
		ind = rand() % 16;
		SET_TILE(test, ind, (GET_TILE(curr, ind)));
		if(!cmpbrd(curr, test)){
			log_out("Wrong!", LOG_ERROR);
			output_board(curr);
			log_out("Not equal", LOG_ERROR);
			output_board(test);
			return false;
		}
		SET_TILE(test, ind, 0);
	}
	log_out("No errors reported", LOG_INFO);
	return true;
}

bool test(void){
	set_log_level(LOG_INFO);
	bool passed = true;
	generate_lut();
	passed &= test_dynamic_arr();
	passed &= test_searching();
	passed &= test_dedupe();
	passed &= test_rots();
	passed &= test_misc();
	passed &= test_settings();
	passed &= test_cmpbrd();
	test_generation();
	if(!passed){
		log_out("One or more tests failed!", LOG_ERROR);
		exit(EXIT_FAILURE);
	}
	else
		log_out("All tests passed!", LOG_INFO);
	return passed;
}
