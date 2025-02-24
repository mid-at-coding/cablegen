#include "../inc/board.h"
#include "../inc/array.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#define DBG
//#define TRACE

uint16_t _move_lut[2][UINT16_MAX];
bool _move_dupe_lut[UINT16_MAX];
bool   _locked_lut[2][UINT16_MAX];

bool flat_move(uint64_t *board, dir d){ // unimplemented
	return false;
};

static bool move(uint64_t *board, const static_arr_info positions, bool free_formation){ // this should not be used directly
	// [ board[pos[0]], board[pos[1]] board[pos[2]] ... board[pos[n]] ] -(left)>
	// starting from the left, greedily merge everything and pull everything along
	// TODO: this is O(n^2); may cause perf issues
	// weird scenario: this does [ f, f, f, f ] -(left)> [ 0, 0, 0, 0 ], idc enough to track it down manually
/*	{	
		bool all32ks = true;
		for(int i = 0; i < positions.size; i++){
			if(GET_TILE((*board), positions.bp[i]) != 0xf)
				all32ks = false;
		}
		if(all32ks){
#ifdef DBG
			printf("all 32ks!\n");
#endif
			return false;
		}
	} */
	uint64_t premove = *board;
	#define SHIFT for(size_t j = i; j < positions.size - 1; j++) { \
				SET_TILE((*board), positions.bp[j], GET_TILE((*board), positions.bp[j + 1])); \
				SET_TILE((*board), positions.bp[j + 1], 0); \
			}
	char merge_candidate = 0;
	bool moved = false;
#ifdef TRACE
	for(size_t i = 0; i < positions.size; i++){
		printf("pos[%lu]: %lu : %u\n", i, positions.bp[i], GET_TILE((*board), positions.bp[i]));
	}
#endif
	for(size_t i = 0; i < positions.size; i++){
		if(i != 0)
			merge_candidate = GET_TILE((*board),positions.bp[i-1]);
		// shift everything with zeros in between
		// check that there are tiles to shift, otherwise we'll be here forever
		char tile = 0;
		for(size_t j = i+1; j < positions.size; j++){
			tile |= GET_TILE((*board), positions.bp[j]);
#ifdef TRACE
			printf("Checking shift, i: %lu, j: %lu\n", i, j);
			printf("Considering: 0x%x\n", GET_TILE((*board), positions.bp[j]));
#endif
		}
		while(tile && GET_TILE((*board), positions.bp[i]) == 0){
			moved = true;
			SHIFT
		}
#ifdef TRACE
		printf("After shift: (i: %lu) 0x%016lx\n", i, *board);
#endif
		if(GET_TILE((*board), positions.bp[i]) == merge_candidate){
			// merge
#ifdef TRACE
			printf("Merging: (i: %lu, mc: %u) 0x%016lx\n", i, merge_candidate, *board);
#endif
			if(merge_candidate != 0xF && GET_TILE((*board), positions.bp[i]) != 0xF){
				moved = true;
				if(merge_candidate != 0)
					SET_TILE((*board), positions.bp[i-1], (merge_candidate + 1));
				SHIFT
			}
		}
#ifdef TRACE
		printf("After merge: (i: %lu) 0x%016lx\n", i, *board);
#endif
	}
	// if this isn't a free formation check if the move is valid
	if(!free_formation){
		for(int i = 0; i < 16; i++){
			if(GET_TILE(premove, i) == 0xf && GET_TILE((*board), i) != 0xf){ // shifted
				*board = premove;
				return false;
			}
		}
	}
	return moved;
}

void generate_lut(bool free_formation){
	bool dupe = false;
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
#ifdef DBG
		printf("Lookup: %016lx\n", tmp_board);
#endif	
		// check if it's a dupe
		dupe = false;
		dupe |= (GET_TILE(tmp_board, 0) == GET_TILE(tmp_board, 1)) && (GET_TILE(tmp_board,2) + GET_TILE(tmp_board,3) == 0); // [x,x,0,0]
		dupe |= (GET_TILE(tmp_board, 0) == GET_TILE(tmp_board, 2)) && (GET_TILE(tmp_board,1) + GET_TILE(tmp_board,3) == 0); // [x,0,x,0]
		dupe |= (GET_TILE(tmp_board, 1) == GET_TILE(tmp_board, 2)) && (GET_TILE(tmp_board,0) + GET_TILE(tmp_board,3) == 0); // [0,x,x,0]
		dupe |= (GET_TILE(tmp_board, 1) == GET_TILE(tmp_board, 3)) && (GET_TILE(tmp_board,0) + GET_TILE(tmp_board,2) == 0); // [0,x,0,x]
		dupe |= (GET_TILE(tmp_board, 2) == GET_TILE(tmp_board, 3)) && (GET_TILE(tmp_board,0) + GET_TILE(tmp_board,1) == 0); // [0,0,x,x]
																															
		dupe |= (GET_TILE(tmp_board, 0) == GET_TILE(tmp_board, 1)) && (GET_TILE(tmp_board,2) == 0); // [x,x,0,y]
		dupe |= (GET_TILE(tmp_board, 0) == GET_TILE(tmp_board, 2)) && (GET_TILE(tmp_board,1) == 0); // [x,0,x,y]
		dupe |= (GET_TILE(tmp_board, 1) == GET_TILE(tmp_board, 2)) && (GET_TILE(tmp_board,0) == 0); // [0,x,x,y]
																									
		dupe |= (GET_TILE(tmp_board, 1) == GET_TILE(tmp_board, 2)) && (GET_TILE(tmp_board,3) == 0); // [y,x,x,0]
		dupe |= (GET_TILE(tmp_board, 1) == GET_TILE(tmp_board, 3)) && (GET_TILE(tmp_board,2) == 0); // [y,x,0,x]
		dupe |= (GET_TILE(tmp_board, 2) == GET_TILE(tmp_board, 3)) && (GET_TILE(tmp_board,1) == 0); // [y,0,x,x]
		premove = tmp_board;
		move(&tmp_board, a, free_formation);

		// if this isn't a free formation check if the move is valid TODO: move these two into a function + clean this up
		if(!free_formation){
			for(int i = 0; i < 4; i++){
				if(GET_TILE(premove, i) == 0xf && GET_TILE((tmp_board), i) != 0xf){ // shifted
					tmp_board = premove;
					_locked_lut[left][i] = false;
				}
			}
		}
		else
			_locked_lut[left][i] = true;
#ifdef DBG
		printf("Res: %016lx\n", tmp_board);
#endif
		_move_lut[left][i] = tmp_board >> 12 * 4;
		_move_dupe_lut[i] = dupe;
#ifdef DBG
		printf("Res: %04x\n", _move_lut[left][i]);
#endif
		SET_TILE(tmp_board, 0, ((i & 0xF000) >> 12));
		SET_TILE(tmp_board, 1, ((i & 0x0F00) >> 8));
		SET_TILE(tmp_board, 2, ((i & 0x00F0) >> 4));
		SET_TILE(tmp_board, 3, (i & 0x000F));
		premove = tmp_board;
		move(&tmp_board, b, free_formation);

		// if this isn't a free formation check if the move is valid TODO: ^^ 
		if(!free_formation){
			for(int i = 0; i < 4; i++){
				if(GET_TILE(premove, i) == 0xf && GET_TILE((tmp_board), i) != 0xf){ // shifted
					tmp_board = premove;
					_locked_lut[right][i] = false;
				}
			}
		}
		else
			_locked_lut[right][i] = true;

		_move_lut[right][i] = tmp_board >> 12 * 4;
		if(i == UINT16_MAX)
			break;
	}
	// i have No clue why _move_lut[left/right][0x0000] == 0xffff
	_move_lut[left][0x0000] = 0x0000;
	_move_lut[right][0x0000] = 0x0000;
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
		if(_locked_lut[direction][lookup]){ // not allowed!!
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

bool move_duplicate(uint64_t board, dir d){
	uint16_t lookup;
	const short BITS_PER_ROW = 16;
	if(d == up || d == down)
		rotate_clockwise(&board); // make sure we're horizontal
	for(int i = 0; i < 4; i++){
		lookup = (board >> BITS_PER_ROW * i) & 0xFFFF; // get a row
		if(!_move_dupe_lut[lookup])
			return false;
	}
	return true;
}
static bool spawn_duplicate_hori(uint64_t board){
	uint16_t lookup;
	const short BITS_PER_ROW = 16;
	for(int i = 0; i < 4; i++){
		lookup = (board >> BITS_PER_ROW * i) & 0xFFFF; // get a row
		if (_move_lut[left][lookup] == 0x2000 || _move_lut[left][lookup] == 0x2){
			return true;
		}
	}
	return false;
}
bool spawn_duplicate(uint64_t board){
	return spawn_duplicate_hori(board) || spawn_duplicate_hori((rotate_counterclockwise(&board), board));
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
	uint64_t nonrot = *board;
	uint64_t one = *board;
	uint64_t two = *board;
	uint64_t three = *board;
	rotate_clockwise(&one);
	rotate_clockwise(&two);
	rotate_clockwise(&two);
	rotate_counterclockwise(&three);
	(*board) = max(nonrot, max(one, max(two, three)));
}

int get_sum(uint64_t b){
	int res = 0;
	for(int i = 0; i < 16; i++){
		int curr = GET_TILE(b,i);
		res += (curr == 0 || curr == 0xf) ? 0 : pow(2,curr);
	}
	return res;
}
