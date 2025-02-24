#ifndef BOARD_H
#define BOARD_H
#include <stdint.h>
#include <stdbool.h>
#define BIT_MASK (0xf000000000000000)
#define OFFSET(x) (x * 4)
#define GET_TILE(b, x) (uint8_t)(((b << OFFSET(x)) & BIT_MASK) >> OFFSET(15))
#define SET_TILE(b, x, v) ( b = (~(~b | (BIT_MASK >> OFFSET(x))) | ((BIT_MASK & ((uint64_t)v << OFFSET(15))) >> OFFSET(x)) ))

enum {
	left,
	right,
	up,
	down
} typedef dir;

extern uint16_t _move_lut[2][UINT16_MAX]; // store every possible row move
extern bool   _locked_lut[2][UINT16_MAX]; // true for all rows that shift: could be 4 bits per lookup but that's probably slower
extern bool   _move_dupe_lut[UINT16_MAX];

void generate_lut(bool free_formation);
void rotate_clockwise(uint64_t*);
void rotate_counterclockwise(uint64_t*);
bool movedir(uint64_t*, dir);
bool flat_move(uint64_t*, dir);
bool spawn_duplicate(uint64_t);
bool move_duplicate(uint64_t, dir);
void canonicalize(uint64_t*);
void normalize(uint64_t*, dir); // make every direction left
int get_sum(uint64_t);

#endif
