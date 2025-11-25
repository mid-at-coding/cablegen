#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include "test.h"
#include "parse.h"
#include "logging.h"
#include "generate.h"
#include "solve.h"
#include "board.h"
#include "settings.h"
#include <string.h>
#ifndef WIN32
#include <termios.h>
#endif
#ifdef WIN32
#include <conio.h>
#endif
#include <errno.h>
#include <time.h>

static size_t find_eq(const char *str){
	for(const char *curr = str; *curr != '\0'; curr++){
		if(*curr == '=')
			return curr - str;
	}
	return 0;
}
static size_t get_eq(char *arg, char *name, char *type){
	size_t eq = find_eq(arg);
	if(!eq){
		printf("Invalid flag! Do %s=%s\n", name, type);
		return 0;
	}
	return eq;
}
static int parseCfg(char *arg, void *data){
	option_t *opt = data;
	size_t eq = get_eq(arg, opt->long_rep, "file");
	if(!eq)
		return -1;
	char *buf = malloc(strlen(arg) - eq);
	if(!buf){
		printf("Could not allocate string buffer for argument!\n");
		return -1;
	}
	strcpy(buf, arg + eq + 1);
	change_config(buf);
	free(buf);
	return 0;
}

static void help(void){
	log_out("Cablegen v1.3 by ember/emelia/cattodoameow", LOG_INFO_);
	log_out("Usage: [cablegen] [flags] [command]", LOG_INFO_);
	log_out("Flags:", LOG_INFO_);
	log_out("-C=[FILE]   --config             -- specifies a config to read flags from, behaviour if unspecified is specified in the README", LOG_INFO_);
	log_out("-f=[bool]   --free-formation     -- specifies whether unmergable (0xf) tiles should be allowed to move", LOG_INFO_);
	log_out("-v=[bool]   --ignore-unmergeable -- treats unmergable tiles as walls", LOG_INFO_);
	log_out("-c=[int]    --cores              -- chooses the amount of cores to use while generating or solving", LOG_INFO_);
	log_out("-n=[int]    --nox                -- treats a tile with this tile on it as dead (0 to disable)", LOG_INFO_);
	log_out("-b=[string] --bdir               -- select a directory for where to put generated positions", LOG_INFO_);
	log_out("-t=[string] --tdir               -- select a directory for where to put solved tables", LOG_INFO_);
	log_out("-i=[string] --initial            -- select a file to start generation from", LOG_INFO_);
	log_out("-e=[int]    --end-gen            -- choose the tile sum to end generation", LOG_INFO_);
	log_out("-E=[int]    --end-solve          -- choose the tile sum to end solving", LOG_INFO_);
	log_out("-w=[string] --winstate           -- select a file to consider winstates from", LOG_INFO_);
	log_out("-d=[bool]   --delete             -- delete positions after solving them", LOG_INFO_);
	log_out("Commands:", LOG_INFO_);
	log_out("help                             -- this output", LOG_INFO_);
	log_out("generate                         -- generate boards", LOG_INFO_);
	log_out("solve                            -- solve boards backwards", LOG_INFO_);
	log_out("generate_solve                   -- generate boards and then solves", LOG_INFO_);
	log_out("test                             -- run self test", LOG_INFO_);
	log_out("read [FILE]                      -- read board(s) from a file", LOG_INFO_);
	log_out("write [FILE] [BOARD] (BOARD) ... -- write board(s) to a file", LOG_INFO_);
	log_out("lookup [BOARD]                   -- look a board up", LOG_INFO_);
	log_out("lookup_spawn [BOARD]             -- look a board up", LOG_INFO_);
	log_out("explore [TABLE]                  -- show all the solved boards in TABLE", LOG_INFO_);
	log_out("train [BOARD]                    -- play a game starting with BOARD, optionally specifying CONFIG, with live feedback", LOG_INFO_);
	log_out("play [BOARD]                     -- simulate an optimal game of BOARD with random spawns", LOG_INFO_);
	log_out("benchmark                        -- benchmark cablegen", LOG_INFO_);
}

static void parseGenerate(){
	set_log_level(LOG_INFO_);
	settings_t settings = *get_settings();
	char *default_postfix = "%d.boards";
	size_t fmt_size = strlen(default_postfix) + strlen(settings.bdir) + 1;
	char *fmt = malloc_errcheck(fmt_size);
	snprintf(fmt, fmt_size, "%s%s", settings.bdir, default_postfix);

	// assume all boards in our file are the same sum (!!)
	static_arr_info boards = read_boards(settings.initial);
	if(boards.size < 1){
		printf("No boards in %s!", settings.initial);
		return;
	}

	if(get_sum(boards.bp[0]) < get_settings()->end_solve){
		log_out("Solving will continue below initial board, consider changing Solve.end!", LOG_WARN_);
	}
	int layer = get_sum(boards.bp[0]);
	generate(layer, settings.end_gen, fmt, boards.bp, boards.size, settings.cores, settings.premove, settings.nox, settings.free_formation);
	free(fmt);
}

static void parseSolve(){
	set_log_level(LOG_INFO_);
	settings_t settings = *get_settings();
	char *default_board_postfix = "%d.boards";
	size_t posfmt_size = strlen(default_board_postfix) + strlen(settings.bdir) + 1;
	char* posfmt = malloc_errcheck(posfmt_size);
	snprintf(posfmt, posfmt_size, "%s%s", settings.bdir, default_board_postfix);

	char* default_table_postfix = "%d.tables";
	size_t table_fmt_size = strlen(default_table_postfix) + strlen(settings.tdir) + 1;
	char* table_fmt = malloc_errcheck(table_fmt_size);
	snprintf(table_fmt, table_fmt_size, "%s%s", settings.tdir, default_table_postfix);

	static_arr_info boards = read_boards(settings.winstates);
	if(boards.size < 1){
		printf("No boards in %s!", settings.initial);
		return;
	}
	for(size_t i = 0; i < boards.size; i++){
		if(get_sum(boards.bp[i]) > get_settings()->end_gen){
			log_out("Winstate will never be generated, consider changing Generate.end!", LOG_WARN_);
			output_board(boards.bp[i]);
		}
	}
	solve(settings.end_gen, settings.end_solve, posfmt, table_fmt, &boards, settings.cores, settings.nox, settings.score, settings.free_formation);
	free(posfmt);
	free(table_fmt);
}

static void parseWrite(int argc, char **argv){
	if(argc < 2){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	dynamic_arr_info boards = init_darr(0, 1);
	for(int i = 2; i < argc; i++){
		uint64_t board = strtoull(argv[i], NULL, 16); // interpret as hex string
		if (errno != 0){
			printf("Error parsing string! %s\n", strerror(errno));
		}
		push_back(&boards, board);
	}
	write_boards((static_arr_info){.valid = boards.valid, .bp = boards.bp, .size = boards.sp - boards.bp}, argv[1], 0);
}

struct dirprob {
	dir d;
	double prob;
};

static char* dirtos(dir d){
	switch(d){
		default:
		case left:
			return "left\0";
		case right:
			return "right\0";
		case up:
			return "up\0";
		case down:
			return "down\0";
	}
}

static struct dirprob best(uint64_t board, table *n){
	uint64_t tmp = board;
	dir maxd = left;
	double maxp = 0;
	double tmpp = 0;
	for(dir d = left; d <= down; d++){
		tmp = board;
		if(!movedir(&tmp, d))
			continue;
		if((tmpp = lookup(tmp, n, true)) > maxp){
			maxd = d;
			maxp = tmpp;
		}
	}
	return (struct dirprob){maxd, maxp};
}

static void parseLookup(int argc, char **argv, bool spawn){ // TODO refactor
	if(argc < 3){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	char *tabledir;
	settings_t settings = *get_settings();
	tabledir = settings.tdir;
	uint64_t board = strtoull(argv[2], NULL, 16); // interpret as hex string
	uint64_t original = board;
	set_log_level(LOG_INFO_);
	if(spawn){ // this board has a spawn on it
		for(int i = 0; i < 16; i++){
			if(GET_TILE(board, i) <= 2){
				SET_TILE(board, i, 0);
				break;
			}
		}
	}
	generate_lut();
	uint64_t sum = get_sum(board);

	char* default_table_postfix = "%d.tables";
	size_t table_fmt_size = strlen(default_table_postfix) + strlen(tabledir) + 10;
	char* table_fmt = malloc_errcheck(table_fmt_size);
	char* tablestr = malloc_errcheck(table_fmt_size);
	snprintf(table_fmt, table_fmt_size, "%s%s", tabledir, default_table_postfix);
	if(!table_fmt){
		log_out("This shouldn't happen", LOG_ERROR_);
		exit(EXIT_FAILURE);
	}
	snprintf(tablestr, table_fmt_size, table_fmt, sum);
	log_out(tablestr, LOG_TRACE_);
	
	table *t  = malloc_errcheck(sizeof(table));
	table *t2 = malloc_errcheck(sizeof(table));
	table *t4 = malloc_errcheck(sizeof(table));
	read_table(t, tablestr);
	double res = lookup(board, t, true);
	snprintf(tablestr, table_fmt_size, table_fmt, sum + 2);
	read_table(t2, tablestr);
	snprintf(tablestr, table_fmt_size, table_fmt, sum + 4);
	read_table(t4, tablestr);


	if(spawn)
		output_board(original);
	else{
		printf("Board(%0.10lf):\n", res);
		output_board(board);
		printf("Spawns:\n");
	}
	for(int i = 0; i < 16; i++){
		if(GET_TILE(board, i))
			continue;
		SET_TILE(board, i, 1);
		struct dirprob pb = best(board, t2);
		if(!spawn || original == board){
			printf("Best move: %s (%.10lf)\n", dirtos(pb.d), pb.prob);
			if(!spawn)
				output_board(board);
		}
		SET_TILE(board, i, 2);
		pb = best(board, t4);
		if(!spawn || original == board){
			printf("Best move: %s (%.10lf)\n", dirtos(pb.d), pb.prob);
			if(!spawn)
				output_board(board);
		}
		SET_TILE(board, i, 0);
	}
	free(table_fmt);
	free(tablestr);
	free(t->key.bp);
	free(t->value.bp);
	free(t);
	free(t2->key.bp);
	free(t2->value.bp);
	free(t2);
	free(t4->key.bp);
	free(t4->value.bp);
	free(t4);
}

static void parseLookupSpawn(int argc, char **argv){
	parseLookup(argc, argv, true);
}
static void parseLookupMove(int argc, char **argv){
	parseLookup(argc, argv, false);
}


static void parseExplore(int argc, char **argv){
	if(argc < 1){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	table *t = malloc_errcheck(sizeof(table));
	read_table(t, argv[1]);
	for(size_t i = 0; i < t->key.size; i++){
		printf("Board(%0.20lf):\n", *(double*)(t->value.bp + i));
		output_board(t->key.bp[i]);
	}
	free(t->key.bp);
	free(t->value.bp);
	free(t);
}

static void parseRead(int argc, char **argv){
	if(argc < 1){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }

	static_arr_info t = read_boards(argv[1]);
	for(size_t i = 0; i < t.size; i++){
		output_board(t.bp[i]);
		if(i + 1 < t.size)
			printf("----\n");
	}
	free(t.bp);
}

static bool canMove(uint64_t board){
	for(dir d = left; d <= down; d++){
		if(movedir(&board, d))
			return true;
	}
	return false;
}

static void spawn(uint64_t *board){
	short spaces = 0;
	for(int i = 0; i < 16; i++){
		if(!GET_TILE((*board), i))
			spaces++;
	}
	if(spaces == 0){
		log_out("No space!", LOG_INFO_);
		output_board(*board);
		exit(0);
	}
	short position = rand() % spaces;
	short tile = rand() % 10;
	short curr = 0;
	for(int i = 0; i < 16; i++){
		if(!GET_TILE((*board), i) && curr++ == position){
			if(tile == 9){
				SET_TILE((*board), i, 2);
			}
			else
				SET_TILE((*board), i, 1);
		}
	}
}
char getch__(void) // https://stackoverflow.com/a/7469410 
{
#ifndef WIN32
	bool echo = false;
	static struct termios current, old;
	tcgetattr(0, &old); /* grab old terminal i/o settings */
	current = old; /* make new settings same as old settings */
	current.c_lflag &= ~ICANON; /* disable buffered i/o */
	if (echo) {
	  current.c_lflag |= ECHO; /* set echo mode */
	} else {
	  current.c_lflag &= ~ECHO; /* set no echo mode */
	}
	tcsetattr(0, TCSANOW, &current); /* use these new terminal i/o settings now */
	char res = getchar();
	tcsetattr(0, TCSANOW, &old);
	return res;
#endif
#ifdef WIN32
	return getch();
#endif
}
static void parseTrain(int argc, char **argv){
	if(argc < 1){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	set_log_level(LOG_INFO_);
	srand(time(0));
	generate_lut();
	double cumacc = 1;
	bool validMove = 0;
	char inp;

	char* default_table_postfix = "%d.tables"; // TODO pull out
	size_t table_fmt_size = strlen(default_table_postfix) + strlen(get_settings()->tdir) + 1;
	char* table_fmt = malloc_errcheck(table_fmt_size);
	snprintf(table_fmt, table_fmt_size, "%s%s", get_settings()->tdir, default_table_postfix);

	size_t table_dir_str_size = strlen(table_fmt) + 10;
	char *table_dir_str = malloc_errcheck(table_dir_str_size);
	table *t  = malloc_errcheck(sizeof(table));
	uint64_t board = strtoull(argv[1], NULL, 16);

	struct dirprob res;
	double tmp = 0;
	log_out("Welcome to cablegen training mode! Move with WASD or HJKL", LOG_INFO_);
	while(canMove(board)){
		printf("-------------\n");
		output_board(board);
		uint64_t premove = board;
		dir move = 0;
		do{
			inp = getch__();
			switch(inp){
			case 'w':
			case 'k':
				move = up;
				validMove = true;
				break;
			case 'a':
			case 'h':
				move = left;
				validMove = true;
				break;
			case 's':
			case 'j':
				move = down;
				validMove = true;
				break;
			case 'd':
			case 'l':
				move = right;
				validMove = true;
				break;
			default:
				validMove = false;
			}
			if(validMove)
				validMove = movedir(&board, move);
			if(validMove == false)
				log_out("Invalid move!", LOG_INFO_);
		}while(!validMove);
		snprintf(table_dir_str, table_dir_str_size, table_fmt, get_sum(board));
		read_table(t, table_dir_str);
		res = best(premove, t);
		if(tmp == 1){
			log_out("You Win!", LOG_INFO_);
			printf("[INFO] Cumulative accuracy: %lf\n", cumacc);
			exit(0);
		}
		if((res.prob - (tmp = lookup(board, t, true))) > 0.005){ // TODO setting?
			log_out("Suboptimal move!", LOG_INFO_);
			printf("[INFO] Table: %lf(%s), You %lf(%s)\n", res.prob, dirtos(res.d), tmp, dirtos(move));
			printf("[INFO] Cumulative accuracy: %lf\n", cumacc *= (tmp/res.prob));
			printf("[INFO] One step loss: %lf\n", 1 - (tmp/res.prob));
		}
		else
			cumacc *= (tmp/res.prob);
		spawn(&board);
		free(t->key.bp);
		free(t->value.bp);
	}
	output_board(board);
	log_out("You Died!", LOG_INFO_);
	printf("[INFO] Cumulative accuracy: %lf\n", cumacc);
	exit(0);
}

static void parsePlay(int argc, char **argv){
	if(argc < 1){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	set_log_level(LOG_INFO_);
	settings_t settings = *get_settings();
	srand(time(NULL));
	char *tdir = settings.tdir;
	uint64_t board = strtoull(argv[1], NULL, 16); // interpret as hex string
	generate_lut();

	char* default_table_postfix = "%d.tables"; // TODO pull out
	size_t table_fmt_size = strlen(default_table_postfix) + strlen(tdir) + 1;
	char* table_fmt = malloc_errcheck(table_fmt_size);
	snprintf(table_fmt, table_fmt_size, "%s%s", tdir, default_table_postfix);

	size_t table_dir_str_size = strlen(table_fmt) + 10;
	char *table_dir_str = malloc_errcheck(table_dir_str_size);
	table *t  = malloc_errcheck(sizeof(table));

	do{
		output_board(board);
		if(!canMove(board)){
			log_out("AI died :(", LOG_INFO_);
			exit(0);
		}
		printf("\n");
		snprintf(table_dir_str, table_dir_str_size, table_fmt, get_sum(board));
		read_table(t, table_dir_str);
		struct dirprob tmp = best(board, t);
		movedir(&board, tmp.d);
		if(tmp.prob == 1.0f){
			log_out("AI won!", LOG_INFO_);
			exit(0);
		}
		free(t->key.bp);
		free(t->value.bp);
		spawn(&board);
	}
	while (true);
	free(table_fmt);
	free(table_dir_str);
	free(t);
}

void benchmark(void){
	dynamic_arr_info initial_arr = init_darr(0,2);
	time_t curr = time(NULL);
	log_out("Benchmarking multi-threaded generation (LL-256)", LOG_INFO_);
	push_back(&initial_arr, 0x1000000021ff12ff);
	push_back(&initial_arr, 0x1000000012ff21ff);
	set_log_level(LOG_WARN_);
	generate(16, 300, ".benchmark/%d.boards", initial_arr.bp, initial_arr.sp - initial_arr.bp, get_settings()->cores, false, false, false);
	set_log_level(LOG_INFO_);
	printf("Multi-threaded LL-256: %d seconds\n", (unsigned)difftime(time(NULL), curr));
	static_arr_info winstates = init_sarr(0,1);
	winstates.bp[0] = 0x0000000008ff0ff;
	log_out("Benchmarking multi-threaded solving (LL-256)", LOG_INFO_);
	curr = time(NULL);
	set_log_level(LOG_WARN_);
	solve(300, 16, ".benchmark/%d.boards", ".benchmark/%d.tables", &winstates, get_settings()->cores, 0, 0, 0);
	set_log_level(LOG_INFO_);
	printf("Multi-threaded LL-256: %d seconds\n", (unsigned)difftime(time(NULL), curr));
}

static void parseGenSolve(){
	parseGenerate();
	parseSolve();
}

static void testThunk(){
	test(); // ignore the bool return value
}

int main(int argc, char **argv){
	set_log_level(LOG_INFO_);
	int commandOffset;
	struct {
		char *name;
		bool args;
		union {
			void (*noargs)(void);
			void (*args)(int, char**);
		} fn;
	} commands[] = {
		{"help", false, .fn.noargs = help},
		{"generate", false, .fn.noargs = parseGenerate},
		{"solve", false, .fn.noargs = parseSolve},
		{"generate_solve", false, .fn.noargs = parseGenSolve},
		{"test", false, .fn.noargs = testThunk},
		{"read", true, .fn.args = parseRead},
		{"write", true, .fn.args = parseWrite},
		{"lookup", true, .fn.args = parseLookupMove},
		{"lookup_spawn", true, .fn.args = parseLookupSpawn},
		{"explore", true, .fn.args = parseExplore},
		{"train", true, .fn.args = parseTrain},
		{"play", true, .fn.args = parsePlay},
		{"benchmark", false, .fn.noargs = benchmark},
	};
	for(int i = 0; i < argc; i++){
		for(size_t cm = 0; cm < sizeof(commands)/sizeof(commands[0]); cm++){
			if(!strcmp(commands[cm].name, argv[i])){
				commandOffset = i;
			}
		}
	}
	option_t opts[] = {
		{ parseCfg , "-C", "--config", get_settings() },
		{ parseBool, "-f", "--free-formation", &get_settings()->free_formation },
		{ parseBool, "-v", "--ignore-unmergeable", &get_settings()->ignore_f },
		{ parseLL  , "-c", "--cores", &get_settings()->cores },
		{ parseLL  , "-n", "--nox", &get_settings()->nox },
		{ parseStr , "-t", "--tdir", &get_settings()->tdir },
		{ parseStr , "-b", "--bdir", &get_settings()->bdir },
		{ parseStr , "-i", "--initial", &get_settings()->initial },
		{ parseLL  , "-e", "--end-gen", &get_settings()->end_gen },
		{ parseLL  , "-E", "--end-solve", &get_settings()->end_solve },
		{ parseStr , "-w", "--winstate", &get_settings()->winstates },
		{ parseBool, "-d", "--delete", &get_settings()->delete_boards }
	};
	parse(opts, sizeof(opts)/sizeof(opts[0]), commandOffset, argv);
	for(size_t i = 0; i < sizeof(commands)/sizeof(commands[0]); i++){
		if(!strcmp(commands[i].name, argv[commandOffset])){
			if(commands[i].args){
				commands[i].fn.args(argc - commandOffset, argv + commandOffset);
			}
			else{
				commands[i].fn.noargs();
			}
		}
	}
	exit(EXIT_SUCCESS);
}
