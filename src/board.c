#include "../inc/board.h"
#include "../inc/array.h"
#include "../inc/logging.h"
#include "../inc/settings.h"
#include <stdint.h>
#include <sys/param.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
//#define DBG
//#define TRACE
#ifdef _WIN32
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

uint16_t _move_lut[2][UINT16_MAX + 1];
bool    _merge_lut[2][UINT16_MAX + 1];
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

static bool shift(uint64_t *board, const static_arr_info positions, const int i){
	bool res = 0;
	char v = 0;
	for(size_t j = i; j + 1 < positions.size; j++){ 
		v = GET_TILE((*board), positions.bp[j + 1]);
		SET_TILE((*board), positions.bp[j], v);
		if(GET_TILE((*board), positions.bp[j+1])){
			res = true;
		}
	}
	SET_TILE((*board), positions.bp[positions.size - 1], 0);
	return res;
}

static bool shiftable(uint64_t *board, const static_arr_info positions, const int i){
	for(size_t j = i; j < positions.size; j++){ 
		if(GET_TILE((*board), positions.bp[j]))
			return true;
	}
	return false;
}

static struct _move_res move(uint64_t *board, static_arr_info positions){ // this should not be used directly
	// [ board[pos[0]], board[pos[1]] board[pos[2]] ... board[pos[n]] ] -(left)>
	size_t positions_size_new = positions.size;
	if(get_settings().ignore_f){
		for(int i = positions.size - 1; i >= 0; i--){
			if(GET_TILE((*board), positions.bp[i]) == 0xf){
				positions_size_new--;
			}
			else
				break;
		}
		positions.size = positions_size_new;
	}
	struct _move_res res = { 0, 0 };
	for(size_t i = 0; i < positions.size; i++){ // "compress" everything
		while(GET_TILE((*board), positions.bp[i]) == 0 && shiftable(board, positions, i)){
			res.changed |= shift(board, positions, i);
		}
	}
	for(size_t i = 0; i + 1 < positions.size; i++){ // merge greedily
		if(GET_TILE((*board), positions.bp[i])){
			if(GET_TILE((*board), positions.bp[i]) == GET_TILE((*board), positions.bp[i + 1]) && GET_TILE((*board), positions.bp[i]) != 0xF){
				res.changed |= shift(board, positions, i);
				if(GET_TILE((*board), positions.bp[i]) == get_settings().smallest_large - 1){
					res.merged = GET_TILE((*board), positions.bp[i]) + 1;
				}
				if((GET_TILE((*board), positions.bp[i])) != 0xE){
					SET_TILE((*board), positions.bp[i], (GET_TILE((*board), positions.bp[i]) + 1));
				}
				else{
					SET_TILE((*board), positions.bp[i], (GET_TILE((*board), positions.bp[i])));
				}
			}
		}
	}
	for(size_t i = 0; i < positions.size; i++){ // compress everything again
		while(GET_TILE((*board), positions.bp[i]) == 0 && shiftable(board, positions, i)){
			res.changed |= shift(board, positions, i);
		}
	}
	return res;
}

void generate_lut(void){
	static_arr_info a; // the tile positions to move; here they are hardcoded to 0,1,2,3 which is moves toward 0(left)
	static_arr_info b; // hardcoded right
	a = init_sarr(false, 4);
	b = init_sarr(false, 4);
	for(int i = 0; i < 4; i++)
		a.bp[i] = b.bp[3 - i] = i;
	for(uint16_t i = 0; true; i++){
		uint64_t tmp_board = 0;
		uint64_t premove = 0;
		SET_TILE(tmp_board, 0, ((i & 0xF000) >> 12));
		SET_TILE(tmp_board, 1, ((i & 0x0F00) >> 8));
		SET_TILE(tmp_board, 2, ((i & 0x00F0) >> 4));
		SET_TILE(tmp_board, 3,  (i & 0x000F));
		premove = tmp_board;
		_merge_lut [left][i] = move(&tmp_board, a).merged;
		_move_lut  [left][i] = tmp_board >> 12 * 4;
		_locked_lut[left][i] = !shifted(premove, tmp_board, get_settings().free_formation);
		tmp_board = premove;
		_merge_lut [right][i] = move(&tmp_board, b).merged;
		_move_lut  [right][i] = tmp_board >> 12 * 4;
		_locked_lut[right][i] = !shifted(premove, tmp_board, get_settings().free_formation);
		if(i == UINT16_MAX)
			break;
	}
	// i have No clue why _move_lut[left/right][0x0000] == 0xffff and vice versa
	_move_lut[left][0x0000] = 0x0000;
	_move_lut[right][0x0000] = 0x0000;
	_move_lut[left][0xFFFF] = 0xFFFF;
	_move_lut[right][0xFFFF] = 0xFFFF;
	free(a.bp);
	free(b.bp);
}

inline static struct _move_res movedir_hori(uint64_t* board, dir direction){
	uint64_t premove = *board;
	uint16_t lookup;
	struct _move_res changed = {false, false};
	const short BITS_PER_ROW = 16;
	for(int i = 0; i < 4; i++){ // 4 rows
		lookup = ((*board) >> BITS_PER_ROW * i) & 0xFFFF; // get a row
		if(_move_lut[direction][lookup] != lookup){
			changed.changed = true;
			changed.merged = _merge_lut[direction][lookup];
			(*board) &= ~((uint64_t)0x000000000000FFFF << BITS_PER_ROW * i); // set the bits we want to set to zero
			(*board) |= ((uint64_t)_move_lut[direction][lookup]) << BITS_PER_ROW * i ; // set them
		}
		if(!_locked_lut[direction][lookup]){ // not allowed!!
			*board = premove;
			return (struct _move_res){false, false};
		}
	}
	return changed;
}

bool movedir(uint64_t* board, dir d){
	bool changed = false;
	// make every direction left
	switch(d){
		case left:
			changed = movedir_hori(board, left).changed;
		break;
		case right:
			changed = movedir_hori(board, right).changed;
		break;
		case up:
			rotate_clockwise(board);
			changed = movedir_hori(board, right).changed;
			rotate_counterclockwise(board); // TODO minor perf improvement if you just.. dont?
		break;
		case down:
			rotate_clockwise(board);
			changed = movedir_hori(board, left).changed;
			rotate_counterclockwise(board);
		break;
	};
	return changed;
}

move_res movedir_mask(uint64_t* board, dir d){
	move_res changed = {false, false};
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
			rotate_counterclockwise(board); // TODO minor perf improvement if you just.. dont?
		break;
		case down:
			rotate_clockwise(board);
			changed = movedir_hori(board, left);
			rotate_counterclockwise(board);
		break;
	};
	return changed;
}
inline void rotate_clockwise(uint64_t* b){ // taken from game-difficult/2048EndgameTablebase (Calculator.py)
    *b = (((*b) & 0xff00ff0000000000) >> 8 ) |
		 (((*b) & 0x00ff00ff00000000) >> 32) |
         (((*b) & 0x00000000ff00ff00) << 32) | 
		 (((*b) & 0x0000000000ff00ff) << 8 );
    *b = (((*b) & 0xf0f00000f0f00000) >> 4 ) |
         (((*b) & 0x0f0f00000f0f0000) >> 16) |
         (((*b) & 0x0000f0f00000f0f0) << 16) |
		 (((*b) & 0x00000f0f00000f0f) << 4 );
}
inline void rotate_counterclockwise(uint64_t* b){
    *b = (((*b) & 0xff00ff0000000000) >> 32) |
		 (((*b) & 0x00ff00ff00000000) << 8 ) |
         (((*b) & 0x00000000ff00ff00) >> 8 ) |
		 (((*b) & 0x0000000000ff00ff) << 32);
    *b = (((*b) & 0xf0f00000f0f00000) >> 16) |
		 (((*b) & 0x0f0f00000f0f0000) << 4 ) |
         (((*b) & 0x0000f0f00000f0f0) >> 4 ) |
		 (((*b) & 0x00000f0f00000f0f) << 16);
}
inline void rotate_180(uint64_t* b){
    (*b) = (((*b) & 0xffffffff00000000) >> 32) | 
		   (((*b) & 0x00000000ffffffff) << 32);
    (*b) = (((*b) & 0xffff0000ffff0000) >> 16) | 
		   (((*b) & 0x0000ffff0000ffff) << 16);
    (*b) = (((*b) & 0xff00ff00ff00ff00) >> 8 ) | 
		   (((*b) & 0x00ff00ff00ff00ff) << 8 );
    (*b) = (((*b) & 0xf0f0f0f0f0f0f0f0) >> 4 ) |
		   (((*b) & 0x0f0f0f0f0f0f0f0f) << 4 );
}

void canonicalize_b(uint64_t* board){ // turn a board into it's canonical version
	uint64_t b = *board;
	uint64_t max = b;
	rotate_clockwise(&b);
	max = MAX(max, b);
	rotate_clockwise(&b);
	max = MAX(max, b);
	rotate_clockwise(&b);
	max = MAX(max, b);
	rotate_clockwise(&b);
	flip(&b);
	max = MAX(max, b);
	rotate_clockwise(&b);
	max = MAX(max, b);
	rotate_clockwise(&b);
	max = MAX(max, b);
	rotate_clockwise(&b);
	max = MAX(max, b);
	*board = max;
}

int get_sum(uint64_t b){
	int res = 0;
	for(int i = 0; i < 16; i++){
		int curr = GET_TILE(b,i);
		res += (curr == 0 || curr == 0xf || curr == 0xe) ? 0 : (1 << curr);
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
	rotate_180(&boards[2]);
	// a^3
	rotate_counterclockwise(&boards[3]);
	// b
	flip(&boards[4]);
	// ba
	flip(&boards[5]);
	rotate_clockwise(&boards[5]);
	// ba^2
	flip(&boards[6]);
	rotate_180(&boards[6]);
	// ba^3
	flip(&boards[7]);
	rotate_counterclockwise(&boards[7]);
	return boards;
}

void flip(uint64_t *board){
    *board = (((*board) & 0xffffffff00000000) >> 32) |
			 (((*board) & 0x00000000ffffffff) << 32);
    *board = (((*board) & 0xffff0000ffff0000) >> 16) |
             (((*board) & 0x0000ffff0000ffff) << 16);
	return;
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

void output_board(uint64_t board){
	char tiles[16][5] = {
		"   \0",
		"2  \0",
		"4  \0",
		"8  \0",
		"16 \0",
		"32 \0",
		"64 \0",
		"128\0",
		"256\0",
		"512\0",
		"1k \0",
		"2k \0",
		"4k \0",
		"8k \0",
		"m  \0",
		"x  \0"
	};
	for(int i = 0; i < 4; i++){
		for(int j = 0; j < 4; j++){
			printf("%s ", tiles[GET_TILE(board, (i * 4 + j))]);	
		}
		printf("\n");
	}
}

#define SETBIT(x, y) (x |= (1 << y))
#define GETBIT(x, y) (x & (1 << y))
void swap(uint64_t *board, char pos1, char pos2){
	char tmp = GET_TILE((*board), pos1);
	SET_TILE((*board), pos1, GET_TILE((*board), pos2));
	SET_TILE((*board), pos2, tmp);
}

void permutations(dynamic_arr_info *d, uint64_t *board, uint16_t n, int i, const static_arr_info *masked_tiles){
	if(i == n){
		push_back(d, *board);
	}
	else{
		for(int j = i; j < n; j++){
			swap(board, masked_tiles->bp[i], masked_tiles->bp[j]);
			permutations(d, board, n, i + 1, masked_tiles);
			swap(board, masked_tiles->bp[i], masked_tiles->bp[j]);
		}
	}
}

dynamic_arr_info unmask_board(uint64_t board, const short smallest_large, unsigned long long sum) {
	const char MASKED_TILE = 0xe;
	uint16_t masked_tile_c = 0;
	static_arr_info masked_tiles = init_sarr(1, 16); // a list of all the indices of the masked tiles
    for(int i = 0; i < 16; i++) {
        short tmp = GET_TILE(board, i);
		if(tmp < MASKED_TILE && tmp > 0) {
			sum -= (1 << tmp);
		}
		else if(tmp == MASKED_TILE){
			masked_tiles.bp[masked_tile_c++] = i;
		}
    } // the sum is guaranteed to be below a 16 bit integer because at most one of each tile is masked, so we can
	  // directly use the sum as a packed bit array of the tiles that are available
	uint16_t set_tiles = sum;
	masked_tiles.size = masked_tile_c;

    size_t fac = 1;
	for(uint16_t i = masked_tile_c; i > 0; i--)
		fac *= i;
    // calculate the factorial of the number of masked tiles: this is the amount of permutations and thus the size of our return array
	dynamic_arr_info result = init_darr(false, fac);
	// generate the base board
	uint64_t base = board;
	uint16_t used = 0;
	for(uint16_t i = 0; i < masked_tiles.size; i++){
		for(int tile = 1; tile < 16; tile++){
			if(GETBIT(set_tiles, tile) && !GETBIT(used, tile)){
				SETBIT(used, tile);
				SET_TILE(base, masked_tiles.bp[i], tile);
				break;
			}
		}
	}
	// recursively find every permutation
	permutations(&result, &base, masked_tiles.size, 0, &masked_tiles);
	free(masked_tiles.bp);
    return result;
}

uint64_t mask_board(uint64_t board, const short smallest_large){
	const short MASK = 0xe;
	uint16_t tiles = 0;
	char tmp = 0;
	for(short tile = 0; tile < 16; tile++){
		if((tmp = GET_TILE(board, tile)) >= smallest_large && tmp != 0xf){
			if(!GETBIT(tiles, tmp)){
				SETBIT(tiles, tmp);
				SET_TILE(board, tile, MASK);
			}
		}
	}
	return board;
}
