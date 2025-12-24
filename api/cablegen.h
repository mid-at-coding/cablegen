#ifndef CABLEGEN_H
#define CABLEGEN_H
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define SETBIT(x, y) (x |= (1 << y)) // doesn't have to be exactly the same
#define CLEARBIT(x, y) (x &= ~(1 << y))
#define GETBIT(x, y) (x & (1 << y))
#define BIT_MASK (0xf000000000000000)
#define OFFSET(x) (x * 4)
#define GET_TILE(b, x) (uint8_t)(((b << OFFSET(x)) & BIT_MASK) >> OFFSET(15))
#define SET_TILE(b, x, v) ( b = (~(~b | (BIT_MASK >> OFFSET(x))) | ((BIT_MASK & ((uint64_t)v << OFFSET(15))) >> OFFSET(x)) ))

#ifndef REMOVE_STRUCT_DECL
typedef struct{
	bool valid;
	uint64_t* bp;
	uint64_t* sp;
	size_t size;
} dynamic_arr_info;

typedef struct{
	bool valid;
	uint64_t* bp;
	size_t size;
} static_arr_info;

typedef struct {
	static_arr_info key;
	static_arr_info value;
} table;

typedef enum {
	left,
	right,
	up,
	down
} dir;
#endif

bool movedir(uint64_t*, dir);
void generate_lut(void);
static_arr_info read_boards(const char *dir);
void change_config(char *cfg);
double lookup(uint64_t key, table *t, bool canonicalize);
typedef struct {
	bool free_formation;
	bool ignore_f;
	long long cores;
	long long nox;
	char *bdir;
	char *initial;
	long long end_gen;
	char *tdir;
	char *winstates;
	long long end_solve;
	bool delete_boards;
} min_settings_t;

min_settings_t *get_settings_min(void);
void init_settings(void);
void generate(const int start, const int end, const char *fmt, const static_arr_info *initial);
void solve(unsigned start, unsigned end, char *posfmt, char *tablefmt, const static_arr_info *winstates);
void* malloc_errcheck(size_t size); // guaranteed to be non-null
dynamic_arr_info init_darr(bool zero, size_t size); // error handling is callee's responsibility
static_arr_info init_sarr(bool zero, size_t size); // error handling is callee's responsibility
static_arr_info shrink_darr(dynamic_arr_info* info); // passed dynamic array becomes invalid
dynamic_arr_info concat(dynamic_arr_info* arr1, dynamic_arr_info* arr2); // both arrays become invalid
void destroy_darr(dynamic_arr_info* arr); // frees and invalidates array
void destroy_sarr(static_arr_info* arr); // frees and invalidates array
bool push_back(dynamic_arr_info*, uint64_t); // returns whether a resize happened
void write_boards(const static_arr_info n, const char* fmt, const int layer);
void read_table(table *t, const char *path);
bool test(void);
int get_sum(uint64_t board);
void output_board(uint64_t);
char *get_version(void); // full version string (i.e. cablegen v1.4)
#endif
