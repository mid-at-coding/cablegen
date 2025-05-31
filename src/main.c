#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include "../inc/test.h"
#include "../inc/logging.h"
#include "../inc/generate.h"
#include "../inc/solve.h"
#include "../inc/board.h"
#include "../inc/settings.h"
#include <string.h>
#include <errno.h>


static int numPlaces (int n) {
    if (n < 0) return numPlaces ((n == INT_MIN) ? INT_MAX: -n) + 1;
    if (n < 10) return 1;
    return 1 + numPlaces (n / 10);
}

static void help(){
	log_out("Cablegen v1.1.2 by ember/emelia/cattodoameow", LOG_INFO_);
	log_out("Commands:", LOG_INFO_);
	log_out("help -- this output", LOG_INFO_);
	log_out("generate (CONFIG) -- generate boards, optionally specifying an alternate config file", LOG_INFO_);
	log_out("solve (CONFIG) -- solve boards backwards, optionally specifying an alternate config file", LOG_INFO_);
	log_out("test -- run self test", LOG_INFO_);
	log_out("read [FILE] -- read board(s) from a file", LOG_INFO_);
	log_out("write [FILE] [BOARD] (BOARD) ... -- write board(s) to a file", LOG_INFO_);
	log_out("lookup [BOARD] (TDIR) -- look a board up, optionally specifying an alternate directory to look in", LOG_INFO_);
	log_out("explore [TABLE] -- show all the solved boards in TABLE", LOG_INFO_);
	log_out("play [BOARD] (TDIR) -- simulate an optimal game of BOARD with random spawns", LOG_INFO_);
}

static void parseGenerate(int argc, char **argv){
	if(argc < 2){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	if(argc > 2){
		change_config(argv[2]);
	}
	settings_t settings = get_settings();
	char *default_postfix = "%d.boards";
	size_t fmt_size = strlen(default_postfix) + strlen(settings.bdir) + 1;
	char *fmt = malloc(fmt_size);
	if(fmt == NULL)
		log_out("Alloc failed!", LOG_ERROR_);
	snprintf(fmt, fmt_size, "%s%s", settings.bdir, default_postfix);

	// assume all boards in FILE are the same sum (!!)
	static_arr_info boards = read_boards(settings.initial);
	if(boards.size < 1){
		printf("No boards in %s!", settings.initial);
	}
	int layer = get_sum(boards.bp[0]);
	generate(layer, settings.end_gen, fmt, boards.bp, boards.size, settings.cores, settings.premove, settings.nox, settings.free_formation);
}

static void parseSolve(int argc, char **argv){
	if(argc < 2){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	if(argc > 2){ 
		change_config(argv[2]);
	}
	settings_t settings = get_settings();
	char *default_board_postfix = "%d.boards";
	size_t posfmt_size = strlen(default_board_postfix) + strlen(settings.bdir) + 1;
	char* posfmt = malloc(posfmt_size);
	if(posfmt == NULL)
		log_out("Alloc failed!", LOG_ERROR_);
	snprintf(posfmt, posfmt_size, "%s%s", settings.bdir, default_board_postfix);

	char* default_table_postfix = "%d.tables";
	size_t table_fmt_size = strlen(default_table_postfix) + strlen(settings.tdir) + 1;
	char* table_fmt = malloc(table_fmt_size);
	if(table_fmt == NULL)
		log_out("Alloc failed!", LOG_ERROR_);
	snprintf(table_fmt, table_fmt_size, "%s%s", settings.tdir, default_table_postfix);

	static_arr_info boards = read_boards(settings.winstates);
	if(boards.size < 1){
		printf("No boards in %s!", argv[6]);
	}
	solve(settings.end_gen, settings.end_solve, posfmt, table_fmt, &boards, settings.cores, settings.nox, settings.score, settings.free_formation);
}

static void parseWrite(int argc, char **argv){
	if(argc < 4){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	dynamic_arr_info boards = init_darr(0, 1);
	for(int i = 3; i < argc; i++){
		uint64_t board = strtoull(argv[i], NULL, 16); // interpret as hex string
		if (errno != 0){
			printf("Error parsing string! %s\n", strerror(errno));
		}
		push_back(&boards, board);
	}
	write_boards((static_arr_info){.valid = boards.valid, .bp = boards.bp, .size = boards.sp - boards.bp}, argv[2], 0);
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

static void parseLookup(int argc, char **argv){ // TODO: this segfaults on bad input
	if(argc < 3){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	char *tabledir;
	settings_t settings = get_settings();
	tabledir = settings.tdir;
	if(argc > 3){ 
		tabledir = argv[3];
	}
	uint64_t board = strtoull(argv[2], NULL, 16); // interpret as hex string
	set_log_level(LOG_INFO_);
	generate_lut(settings.free_formation); 
	uint64_t sum = get_sum(board);

	char* default_table_postfix = "%d.tables";
	size_t table_fmt_size = strlen(default_table_postfix) + strlen(argv[2]) + 10;
	char* table_fmt = malloc(table_fmt_size);
	char* tablestr = malloc(table_fmt_size);
	if(table_fmt == NULL)
		log_out("Alloc failed!", LOG_ERROR_);
	snprintf(table_fmt, table_fmt_size, "%s%s", argv[2], default_table_postfix);
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


	printf("Board(%lf):\n", res);
	output_board(board);
	printf("Spawns:\n");
	for(int i = 0; i < 16; i++){
		if(GET_TILE(board, i))
			continue;
		SET_TILE(board, i, 1);
		struct dirprob pb = best(board, t2);
		printf("Best move: %s (%lf)\n", dirtos(pb.d), pb.prob);
		output_board(board);
		SET_TILE(board, i, 2);
		pb = best(board, t4);
		printf("Best move: %s (%lf)\n", dirtos(pb.d), pb.prob);
		output_board(board);
		SET_TILE(board, i, 0);
	}

}

static void parseExplore(int argc, char **argv){
	if(argc < 3){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	table *t = malloc_errcheck(sizeof(table));
	read_table(t, argv[2]);
	for(size_t i = 0; i < t->key.size; i++){
		printf("Board(%lf):\n", *(double*)(t->value.bp + i));
		output_board(t->key.bp[i]);
	}
}

static void parseRead(int argc, char **argv){
	if(argc < 3){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }

	static_arr_info t = read_boards(argv[2]);
	for(size_t i = 0; i < t.size; i++){
		output_board(t.bp[i]);
		if(i + 1 < t.size)
			printf("----\n");
	}
}

static bool canMove(uint64_t board){
	for(dir d = left; d <= down; d++){
		if(movedir(&board, d))
			return true;
	}
	return false;
}

static void parsePlay(int argc, char **argv){
	if(argc < 3){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	set_log_level(LOG_INFO_);
	settings_t settings = get_settings();
	srand(time(NULL));
	char *tdir = settings.tdir;
	if(argc > 3){
		tdir = argv[3];
	}
	uint64_t board = strtoull(argv[2], NULL, 16); // interpret as hex string
	generate_lut(settings.free_formation);

	char* default_table_postfix = "%d.tables";
	size_t table_fmt_size = strlen(default_table_postfix) + strlen(tdir) + 1;
	char* table_fmt = malloc(table_fmt_size);
	if(table_fmt == NULL)
		log_out("Alloc failed!", LOG_ERROR_);
	snprintf(table_fmt, table_fmt_size, "%s%s", tdir, default_table_postfix);

	size_t table_dir_str_size = strlen(table_fmt) + 10;
	char *table_dir_str = malloc_errcheck(table_dir_str_size);
	table *t  = malloc_errcheck(sizeof(table));

	do{
		// spawn
		short spaces = 0;
		for(int i = 0; i < 16; i++){
			if(!GET_TILE(board, i))
				spaces++;
		}
		short position = rand() % spaces;
		short tile = rand() % 10;
		short curr = 0;
		for(int i = 0; i < 16; i++){
			if(!GET_TILE(board, i) && curr++ == position){
				if(tile == 9){
					SET_TILE(board, i, 2);
				}
				else
					SET_TILE(board, i, 1);
			}
		}
		output_board(board);
		if(!canMove(board)){
			log_out("AI died :(", LOG_INFO_);
			exit(0);
		}
		printf("\n");
		snprintf(table_dir_str, table_dir_str_size, table_fmt, get_sum(board));
		read_table(t, table_dir_str);
		movedir(&board, best(board, t).d);
	}
	while (true);
}

int main(int argc, char **argv){
	// parse arguments
	if(argc > 1){
		if(!strcmp(argv[1],"help")) {help();}
		else if(!strcmp(strlwr_(argv[1]), "generate")) {parseGenerate(argc, argv);}
		else if(!strcmp(strlwr_(argv[1]), "solve")) {parseSolve(argc, argv);}
		else if(!strcmp(strlwr_(argv[1]), "test")) {test();}
		else if(!strcmp(strlwr_(argv[1]), "read")) {parseRead(argc, argv);}
		else if(!strcmp(strlwr_(argv[1]), "write")) {parseWrite(argc, argv);}
		else if(!strcmp(strlwr_(argv[1]), "lookup")) {parseLookup(argc, argv);}
		else if(!strcmp(strlwr_(argv[1]), "explore")) {parseExplore(argc, argv);}
		else if(!strcmp(strlwr_(argv[1]), "play")) {parsePlay(argc, argv);}
		else{
			log_out("Unrecognized command!", LOG_WARN_);
			help();
		}
	}
	else{
		help();
	}
	return 0;
}
