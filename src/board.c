#include "../inc/board.h"
#include "../inc/array.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
//#define DBG
//#define TRACE

uint16_t _move_lut[2][UINT16_MAX + 1];
bool   _locked_lut[2][UINT16_MAX + 1];

static bool shifted(uint64_t board, uint64_t board2, bool free_formation){
	if(!free_formation){
		for(int i = 0; i < 4; i++){
		    if(GET_TILE(board,i) == 0xf && GET_TILE(board2,i) != 0xf){
				return true;
		    }
		}
	}
	return false;
}

bool flat_move(uint64_t *board, dir d){ // unimplemented
	return false;
};

static bool shift(uint64_t *board, const static_arr_info positions, const int i){
	bool res = 0;
	for(int j = i; j + 1 < positions.size; j++){ 
		SET_TILE((*board), positions.bp[j], GET_TILE((*board), positions.bp[j + 1])); 
		if(GET_TILE((*board), positions.bp[j+1])){
			res = true;
		}
	}
	SET_TILE((*board), positions.bp[positions.size - 1], 0);
	return res;
}

static bool shiftable(uint64_t *board, const static_arr_info positions, const int i){
	for(int j = i; j < positions.size; j++){ 
		if(GET_TILE((*board), positions.bp[j]))
			return true;
	}
	return false;
}

static bool move(uint64_t *board, const static_arr_info positions, bool free_formation){ // this should not be used directly
	// [ board[pos[0]], board[pos[1]] board[pos[2]] ... board[pos[n]] ] -(left)>
	bool res = false;
	for(int i = 0; i < positions.size; i++){ // "compress" everything
		while(GET_TILE((*board), positions.bp[i]) == 0 && shiftable(board, positions, i)){
			res |= shift(board, positions, i);
		}
	}
	for(int i = 0; i + 1 < positions.size; i++){ // merge greedily
		if(GET_TILE((*board), positions.bp[i])){
			if(GET_TILE((*board), positions.bp[i]) == GET_TILE((*board), positions.bp[i + 1]) && GET_TILE((*board), positions.bp[i]) != 0xF){
				res |= shift(board, positions, i);
				SET_TILE((*board), positions.bp[i], (GET_TILE((*board), positions.bp[i]) + 1));
			}
		}
	}
	for(int i = 0; i < positions.size; i++){ // compress everything again
		while(GET_TILE((*board), positions.bp[i]) == 0 && shiftable(board, positions, i)){
			res |= shift(board, positions, i);
		}
	}
	return res;
}

void generate_lut(bool free_formation){
	static_arr_info a; // the tile positions to move; here they are hardcoded to 0,1,2,3 which is moves toward 0(left)
	static_arr_info b; // the tile positions to move; here they are hardcoded to 3,2,1,0 which is moves toward 3(right)
	a = init_sarr(false, 4);
	a.bp[0] = 0;
	a.bp[1] = 1;
	a.bp[2] = 2;
	a.bp[3] = 3;
	b = init_sarr(false, 4);
	b.bp[0] = 3;
	b.bp[1] = 2;
	b.bp[2] = 1;
	b.bp[3] = 0;
	for(uint16_t i = 0; i <= UINT16_MAX; i++){
		uint64_t tmp_board = 0;
		uint64_t premove = 0;
		SET_TILE(tmp_board, 0, ((i & 0xF000) >> 12));
		SET_TILE(tmp_board, 1, ((i & 0x0F00) >> 8));
		SET_TILE(tmp_board, 2, ((i & 0x00F0) >> 4));
		SET_TILE(tmp_board, 3, (i & 0x000F));
#ifdef TRACE
		printf("Lookup: %016lx\n", tmp_board);
#endif	
		premove = tmp_board;
		move(&tmp_board, a, free_formation);
#ifdef TRACE
		printf("Res: %016lx\n", tmp_board);
#endif
		_move_lut[left][i] = tmp_board >> 12 * 4;
		_locked_lut[left][i] = !shifted(premove, tmp_board, free_formation);
#ifdef TRACE
		printf("Res: %04x\n", _move_lut[left][i]);
#endif
		SET_TILE(tmp_board, 0, ((i & 0xF000) >> 12));
		SET_TILE(tmp_board, 1, ((i & 0x0F00) >> 8));
		SET_TILE(tmp_board, 2, ((i & 0x00F0) >> 4));
		SET_TILE(tmp_board, 3, (i & 0x000F));
		premove = tmp_board;
		move(&tmp_board, b, free_formation);
		_move_lut[right][i] = tmp_board >> 12 * 4;
		_locked_lut[right][i] = !shifted(premove, tmp_board, free_formation);
		if(i == UINT16_MAX)
			break;
	}
	// i have No clue why _move_lut[left/right][0x0000] == 0xffff and vice versa
	_move_lut[left][0x0000] = 0x0000;
	_move_lut[right][0x0000] = 0x0000;
	_move_lut[left][0xFFFF] = 0xFFFF;
	_move_lut[right][0xFFFF] = 0xFFFF;
	free(a.bp);
}

bool movedir_hori(uint64_t* board, dir direction){
	uint64_t premove = *board;
	uint16_t lookup;
	bool changed = false;
	const short BITS_PER_ROW = 16;
	for(int i = 0; i < 4; i++){ // 4 rows
		lookup = ((*board) >> BITS_PER_ROW * i) & 0xFFFF; // get a row
#ifdef DBG
		printf("Lookup: %04x\n", lookup);
#endif
		if(_move_lut[direction][lookup] != lookup){
			changed = true;
			(*board) &= ~((uint64_t)0x000000000000FFFF << BITS_PER_ROW * i); // set the bits we want to set to zero
			(*board) |= ((uint64_t)_move_lut[direction][lookup]) << BITS_PER_ROW * i ; // set them
#ifdef DBG
			printf("Res: %04x\n", _move_lut[direction][lookup]);
#endif
		}
		if(!_locked_lut[direction][lookup]){ // not allowed!!
#ifdef DBG
			printf("Lookup: %04x, %b not allowed!\n", lookup, direction);
#endif
			*board = premove;
			return false;
		}
	}
	return changed;
}

bool movedir(uint64_t* board, dir d){
	bool changed = false;
	// make every direction left
	switch(d){
		case left:
			changed = movedir_hori(board, left);
		break;
		case right:
			changed = movedir_hori(board, right);
		break;
		case up:
			rotate_clockwise(board);
			changed = movedir_hori(board, right);
			rotate_counterclockwise(board);
		break;
		case down:
			rotate_clockwise(board);
			changed = movedir_hori(board, left);
			rotate_counterclockwise(board);
		break;
	};
	// split up board and look up
	return changed;
}

void rotate_clockwise(uint64_t* b){ // there's probably some trick to do this faster but lut lookup is probably not a bottleneck
	uint64_t tmp = *b;
	for(int i = 0; i < 4; i++){
		SET_TILE((*b),(4 * i + 0),(GET_TILE(tmp,(i + 12))));
		SET_TILE((*b),(4 * i + 1),(GET_TILE(tmp,(i + 8))));
		SET_TILE((*b),(4 * i + 2),(GET_TILE(tmp,(i + 4))));
		SET_TILE((*b),(4 * i + 3),(GET_TILE(tmp,(i + 0))));
	}
}
void rotate_counterclockwise(uint64_t* b){
	uint64_t tmp = *b;
	for(int i = 0; i < 4; i++){
		SET_TILE((*b),(4 * i + 0),(GET_TILE(tmp,(3  - i))));
		SET_TILE((*b),(4 * i + 1),(GET_TILE(tmp,(7  - i))));
		SET_TILE((*b),(4 * i + 2),(GET_TILE(tmp,(11 - i))));
		SET_TILE((*b),(4 * i + 3),(GET_TILE(tmp,(15 - i))));
	}
}

static uint64_t max(uint64_t a, uint64_t b){
	if (b > a)
		return b;
	return a;
}

void canonicalize(uint64_t* board){ // turn a board into it's canonical version
	uint64_t *rots = get_all_rots(*board);
	for(int i = 0; i < 8; i++){
		if(rots[i] > *board)
			*board = rots[i];
	}
}

int get_sum(uint64_t b){
	int res = 0;
	for(int i = 0; i < 16; i++){
		int curr = GET_TILE(b,i);
		res += (curr == 0 || curr == 0xf) ? 0 : pow(2,curr);
	}
	return res;
}

uint64_t *get_all_rots(uint64_t board){
	uint64_t *boards = malloc_errcheck(8 * sizeof(uint64_t));
	for(int i = 0; i < 8; i++){
		boards[i] = board;
	}
	// e
	boards[0] = board;
	// a
	rotate_clockwise(&boards[1]);
	// a^2
	rotate_clockwise(&boards[2]);
	rotate_clockwise(&boards[2]);
	// a^3
	rotate_counterclockwise(&boards[3]);
	// b
	flip(&boards[4]);
	// ba
	flip(&boards[5]);
	rotate_clockwise(&boards[5]);
	// ba^2
	flip(&boards[6]);
	rotate_clockwise(&boards[6]);
	rotate_clockwise(&boards[6]);
	// ba^3
	flip(&boards[7]);
	rotate_counterclockwise(&boards[7]);
	return boards;
}

void flip(uint64_t *board){
	// 0  1  2  3
	// 4  5  6  7
	// 8  9  10 11
	// 12 13 14 15
	uint64_t tmp = *board;
	for(int i = 0; i < 4; i++){
		SET_TILE((*board),(4 * i + 0),(GET_TILE(tmp,(3  + 4 * i))));
		SET_TILE((*board),(4 * i + 1),(GET_TILE(tmp,(2  + 4 * i))));
		SET_TILE((*board),(4 * i + 2),(GET_TILE(tmp,(1  + 4 * i))));
		SET_TILE((*board),(4 * i + 3),(GET_TILE(tmp,(0  + 4 * i))));
	}
}
