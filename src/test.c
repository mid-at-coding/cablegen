#include "../inc/generate.h"
#include "../inc/board.h"
#include <stdio.h>

bool test_dynamic_arr(void){
	bool passed = true;
	dynamic_arr_info tmp;
	printf("Testing initialization (n = [0,50])\n");
	for(size_t i = 0; i < 50; i++){
		tmp = init_darr(false, i);
		for(size_t j = 0; j < i; j++){
			tmp.bp[j] = 0; // test that we do really have access to all elements
		}
		free(tmp.bp);
	}
	printf("Testing resizing (n = [0,50])\n");
	for(size_t i = 0; i < 50; i++){
		tmp = init_darr(false, 0);
		for(size_t j = 0; j < i; j++){
			push_back(&tmp, 0);
		}
		free(tmp.bp);
	}
	printf("Testing concatenation (n,m = [0,50])\n");
	for(size_t n = 0; n < 50; n++){
		for(size_t m = 0; m < 50; m++){
			dynamic_arr_info tmpn = init_darr(false, n);
			dynamic_arr_info tmpm = init_darr(false, m);
			for(size_t i = 0; i < n; i++){
				push_back(&tmpn, 0);
			}
			for(size_t i = 0; i < m; i++){
				push_back(&tmpm, 0);
			}
			dynamic_arr_info nm = concat(&tmpn, &tmpm);
			if(nm.sp - nm.bp != n + m){
				printf("Concatenation test failed! n: %zu, m: %zu\n", n, m);
				passed = false;
			}
			free(nm.bp);
			nm.bp = NULL;
			nm.sp = NULL;
			nm.size = 0;
		}
	}
	if(passed){
		printf("No errors reported.\n");
	}
	return passed;
}

void test_generation(){
	printf("Testing generation (correctness not checked).\n");
	dynamic_arr_info n = init_darr(false, 0);
	push_back(&n, 0x1000002000000000); // board with a 2 and a 4 in a kinda arbitrary position
	dynamic_arr_info n2 = init_darr(false, 0);
	dynamic_arr_info n4 = init_darr(false, 0);
	dynamic_arr_info pd = init_darr(false, 0);
	generate_layer("/dev/null", &n, &n2, &n4, &pd, 1);
	printf("Done testing generation.\n");
}

void test_potential_dupe(){
	
}

bool test(){
	bool passed = true;
	generate_lut(true);
	if(!test_dynamic_arr())
		passed = false;
	test_potential_dupe();
	test_generation();
	return passed;
}
