#include <raylib.h>
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
#include <ctype.h>

static char *strlwr(char *str) {
  unsigned char *p = (unsigned char *)str;
  while (*p) {
     *p = tolower((unsigned char)*p);
      p++;
  }
  return str;
}
static int numPlaces (int n) {
    if (n < 0) return numPlaces ((n == INT_MIN) ? INT_MAX: -n) + 1;
    if (n < 10) return 1;
    return 1 + numPlaces (n / 10);
}

static void help(){
	log_out("Cablegen v1.0 by ember/emelia/cattodoameow", LOG_INFO_);
	log_out("Commands:", LOG_INFO_);
	log_out("help -- this output", LOG_INFO_);
	log_out("generate [FILE] [DIR] [END] [CORES] -- generate all sub-boards of the boards in FILE in DIR until the tile sum meets END, using CORES cores", LOG_INFO_);
	log_out("solve [BDIR] [TDIR] [START] [END] [WINSTATE] [CORES] -- solve all boards in BDIR backwards, putting the solutions in TDIR", LOG_INFO_);
	log_out("test -- run self test", LOG_INFO_);
	log_out("write [FILE] [BOARD] (BOARD) ... -- write board(s) to a file", LOG_INFO_);
	log_out("lookup [TDIR] [BOARD] -- look a board up in TDIR", LOG_INFO_);
	log_out("explore [TABLE] -- show all the boards in TABLE", LOG_INFO_);
}

static void parseGenerate(int argc, char **argv){
	if(argc < 6){ log_out("Not enough arguments!", LOG_ERROR_); return;}
	char* default_postfix = "%d.boards";
	size_t fmt_size = strlen(default_postfix) + strlen(argv[3]) + 1;
	char* fmt = malloc(fmt_size);
	if(fmt == NULL)
		log_out("Alloc failed!", LOG_ERROR_);
	snprintf(fmt, fmt_size, "%s%s", argv[3], default_postfix);

	// assume all boards in FILE are the same sum (!!)
	static_arr_info boards = read_boards(argv[2]);
	if(boards.size < 1){
		printf("No boards in %s!", argv[2]);
	}
	int layer = get_sum(boards.bp[0]);
	int end = atoi(argv[4]); 
	if (errno != 0){
		printf("Error parsing end! %s\n", strerror(errno));
	}
	int core_count = atoi(argv[5]); 
	if (errno != 0){
		printf("Error parsing core count! %s\n", strerror(errno));
	}
	generate(layer, end, fmt, boards.bp, boards.size, core_count, false);
}

static void parseSolve(int argc, char **argv){
	if(argc < 8){ log_out("Not enough arguments!", LOG_ERROR_); return;}
	// solve [BDIR] [TDIR] [START] [END] [WINSTATE] [CORES] -- solve all boards in BDIR backwards, putting the solutions in TDIR
	char* default_board_postfix = "%d.boards";
	size_t posfmt_size = strlen(default_board_postfix) + strlen(argv[2]) + 1;
	char* posfmt = malloc(posfmt_size);
	if(posfmt == NULL)
		log_out("Alloc failed!", LOG_ERROR_);
	snprintf(posfmt, posfmt_size, "%s%s", argv[2], default_board_postfix);

	char* default_table_postfix = "%d.tables";
	size_t table_fmt_size = strlen(default_table_postfix) + strlen(argv[3]) + 1;
	char* table_fmt = malloc(table_fmt_size);
	if(table_fmt == NULL)
		log_out("Alloc failed!", LOG_ERROR_);
	snprintf(table_fmt, table_fmt_size, "%s%s", argv[3], default_table_postfix);

	int start = atoi(argv[4]); 
	if (errno != 0){
		printf("Error parsing start! %s\n", strerror(errno));
	}
	int end = atoi(argv[5]); 
	if (errno != 0){
		printf("Error parsing end! %s\n", strerror(errno));
	} // TODO refactor this !!
	int cores = atoi(argv[7]);  // unused
	if (errno != 0){
		printf("Error parsing core count! %s\n", strerror(errno));
	} 
	static_arr_info boards = read_boards(argv[6]);
	if(boards.size < 1){
		printf("No boards in %s!", argv[6]);
	}
	solve(start, end, posfmt, table_fmt, &boards, cores);
}

static void parseWrite(int argc, char **argv){
	if(argc < 4){ log_out("Not enough arguments!", LOG_ERROR_); return;}
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


static void parseLookup(int argc, char **argv){
	if(argc < 4){ log_out("Not enough arguments!", LOG_ERROR_); return;}
	uint64_t board = strtoull(argv[3], NULL, 16); // interpret as hex string
	if (errno != 0){
		printf("Error parsing string! %s\n", strerror(errno));
	}
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
	
	table *t = malloc_errcheck(sizeof(table));
	read_table(t, tablestr);
	double res = lookup(board, t);


	printf("Board(%lf):\n", res);
	output_board(board);
}

static void parseExplore(int argc, char **argv){
	if(argc < 3){ log_out("Not enough arguments!", LOG_ERROR_); return;}

	table *t = malloc_errcheck(sizeof(table));
	read_table(t, argv[2]);
	for(int i = 0; i < t->key.size; i++){
		printf("Board(%lf):\n", *(double*)(t->value.bp + i));
		output_board(t->key.bp[i]);
	}
}

int main(int argc, char **argv){
	// parse arguments
	// valid commands:
	// 	help
	// 	generate [board file] [dir] [end]
	// 	solve [board dir] [table dir] [start] [end]
	// 	write [board file] [board] ...
	if(argc > 1){
		if(!strcmp(argv[1],"help")) {help();}
		else if(!strcmp(strlwr(argv[1]), "generate")) {parseGenerate(argc, argv);}
		else if(!strcmp(strlwr(argv[1]), "solve")) {parseSolve(argc, argv);}
		else if(!strcmp(strlwr(argv[1]), "test")) {test();}
		else if(!strcmp(strlwr(argv[1]), "write")) {parseWrite(argc, argv);}
		else if(!strcmp(strlwr(argv[1]), "lookup")) {parseLookup(argc, argv);}
		else if(!strcmp(strlwr(argv[1]), "explore")) {parseExplore(argc, argv);}
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
