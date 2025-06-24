#ifndef BOARD_H
#define BOARD_H
#include <stdint.h>
#include <stdbool.h>
#include "array.h"
#define BIT_MASK (0xf000000000000000)
#define OFFSET(x) (x * 4)
#define GET_TILE(b, x) (uint8_t)(((b << OFFSET(x)) & BIT_MASK) >> OFFSET(15))
#define SET_TILE(b, x, v) ( b = (~(~b | (BIT_MASK >> OFFSET(x))) | ((BIT_MASK & ((uint64_t)v << OFFSET(15))) >> OFFSET(x)) ))

typedef enum {
	left,
	right,
	up,
	down
} dir;

typedef struct {
	uint64_t board[8];
} symmetries;

extern uint16_t _move_lut[2][UINT16_MAX + 1]; // store every possible row move
extern bool   _locked_lut[2][UINT16_MAX + 1]; // true for all rows that shift: could be 4 bits per lookup but that's probably slower

void generate_lut(bool free_formation);
void rotate_clockwise(uint64_t*);
void rotate_counterclockwise(uint64_t*);
void rotate_180(uint64_t*);
void flip(uint64_t*);
bool movedir(uint64_t*, dir);
bool flat_move(uint64_t*, dir);
void canonicalize_b(uint64_t*);
void normalize(uint64_t*, dir); // make every direction left
int get_sum(uint64_t);
uint64_t *get_all_rots(uint64_t);
void output_board(uint64_t);
dynamic_arr_info unmask_board(uint64_t board, const short smallest_large, long long sum);
dynamic_arr_info unmask_board_recursive(uint64_t board, const short smallest_large, long remaining, short start);
uint64_t mask_board(uint64_t, const short smallest_large);

#endif
