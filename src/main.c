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
#include <termios.h>
#include <errno.h>
#include <time.h>

static void help(void){
	log_out("Cablegen v1.2 by ember/emelia/cattodoameow", LOG_INFO_);
	log_out("Commands:", LOG_INFO_);
	log_out("help -- this output", LOG_INFO_);
	log_out("generate (CONFIG) -- generate boards, optionally specifying an alternate config file", LOG_INFO_);
	log_out("solve (CONFIG) -- solve boards backwards, optionally specifying an alternate config file", LOG_INFO_);
	log_out("test -- run self test", LOG_INFO_);
	log_out("read [FILE] -- read board(s) from a file", LOG_INFO_);
	log_out("write [FILE] [BOARD] (BOARD) ... -- write board(s) to a file", LOG_INFO_);
	log_out("lookup [BOARD] (TDIR) -- look a board up, optionally specifying an alternate directory to look in", LOG_INFO_);
	log_out("lookup_spawn [BOARD] (TDIR) -- look a board up, optionally specifying an alternate directory to look in", LOG_INFO_);
	log_out("explore [TABLE] -- show all the solved boards in TABLE", LOG_INFO_);
	log_out("train [BOARD] (CONFIG) -- play a game starting with BOARD, optionally specifying CONFIG, with live feedback", LOG_INFO_);
	log_out("play [BOARD] (TDIR) -- simulate an optimal game of BOARD with random spawns", LOG_INFO_);
	log_out("benchmark -- benchmark cablegen", LOG_INFO_);
}

static void parseGenerate(int argc, char **argv){
	set_log_level(LOG_INFO_);
	if(argc < 2){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	if(argc > 2){
		change_config(argv[2]);
	}
	settings_t settings = get_settings();
	char *default_postfix = "%d.boards";
	size_t fmt_size = strlen(default_postfix) + strlen(settings.bdir) + 1;
	char *fmt = malloc_errcheck(fmt_size);
	snprintf(fmt, fmt_size, "%s%s", settings.bdir, default_postfix);

	// assume all boards in FILE are the same sum (!!)
	static_arr_info boards = read_boards(settings.initial);
	if(boards.size < 1){
		printf("No boards in %s!", settings.initial);
	}
	int layer = get_sum(boards.bp[0]);
	generate(layer, settings.end_gen, fmt, boards.bp, boards.size, settings.cores, settings.premove, settings.nox, settings.free_formation);
	free(fmt);
}

static void parseSolve(int argc, char **argv){
	set_log_level(LOG_INFO_);
	if(argc < 2){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	if(argc > 2){ 
		change_config(argv[2]);
	}
	settings_t settings = get_settings();
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
		printf("No boards in %s!", argv[6]);
	}
	solve(settings.end_gen, settings.end_solve, posfmt, table_fmt, &boards, settings.cores, settings.nox, settings.score, settings.free_formation);
	free(posfmt);
	free(table_fmt);
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

static void parseLookup(int argc, char **argv, bool spawn){ // TODO refactor
	if(argc < 3){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	char *tabledir;
	settings_t settings = get_settings();
	tabledir = settings.tdir;
	if(argc > 3){ 
		tabledir = argv[3];
	}
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

static void parseExplore(int argc, char **argv){
	if(argc < 3){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	table *t = malloc_errcheck(sizeof(table));
	read_table(t, argv[2]);
	for(size_t i = 0; i < t->key.size; i++){
		printf("Board(%0.20lf):\n", *(double*)(t->value.bp + i));
		output_board(t->key.bp[i]);
	}
	free(t->key.bp);
	free(t->value.bp);
	free(t);
}

static void parseRead(int argc, char **argv){
	if(argc < 3){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }

	static_arr_info t = read_boards(argv[2]);
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
}
static void parseTrain(int argc, char **argv){
	if(argc < 3){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	set_log_level(LOG_INFO_);
	if(argc > 3)
		change_config(argv[3]);
	srand(time(0));
	generate_lut();
	double cumacc = 1;
	bool validMove = 0;
	char inp;

	char* default_table_postfix = "%d.tables"; // TODO pull out
	size_t table_fmt_size = strlen(default_table_postfix) + strlen(get_settings().tdir) + 1;
	char* table_fmt = malloc_errcheck(table_fmt_size);
	snprintf(table_fmt, table_fmt_size, "%s%s", get_settings().tdir, default_table_postfix);

	size_t table_dir_str_size = strlen(table_fmt) + 10;
	char *table_dir_str = malloc_errcheck(table_dir_str_size);
	table *t  = malloc_errcheck(sizeof(table));
	uint64_t board = strtoull(argv[2], NULL, 16);

	struct dirprob res;
	double tmp = 0;
	log_out("Welcome to cablegen training mode! Move with WASD or HJKL", LOG_INFO_);
	while(canMove(board)){
		printf("-------------\n");
		output_board(board);
		validMove = true;
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
	if(argc < 3){ log_out("Not enough arguments!", LOG_ERROR_); help(); exit(1); }
	set_log_level(LOG_INFO_);
	settings_t settings = get_settings();
	srand(time(NULL));
	char *tdir = settings.tdir;
	if(argc > 3){
		tdir = argv[3];
	}
	uint64_t board = strtoull(argv[2], NULL, 16); // interpret as hex string
	generate_lut();

	char* default_table_postfix = "%d.tables"; // TODO pull out
	size_t table_fmt_size = strlen(default_table_postfix) + strlen(tdir) + 1;
	char* table_fmt = malloc_errcheck(table_fmt_size);
	snprintf(table_fmt, table_fmt_size, "%s%s", tdir, default_table_postfix);

	size_t table_dir_str_size = strlen(table_fmt) + 10;
	char *table_dir_str = malloc_errcheck(table_dir_str_size);
	table *t  = malloc_errcheck(sizeof(table));

	do{
		spawn(&board);
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
	}
	while (true);
	free(table_fmt);
	free(table_dir_str);
	free(t);
}

void benchmark(void){
	dynamic_arr_info initial_arr = init_darr(0,2);
	time_t curr = time(NULL);
	log_out("Benchmarking multi-threaded generation (LL-128)", LOG_INFO_);
	push_back(&initial_arr, 0x1000000021ff12ff);
	push_back(&initial_arr, 0x1000000012ff21ff);
	set_log_level(LOG_WARN_);
	generate(16, 150, "/tmp/cablegen/%d.boards", initial_arr.bp, initial_arr.sp - initial_arr.bp, get_settings().cores, false, false, false);
	set_log_level(LOG_INFO_);
	printf("Multi-threaded LL-128: %d seconds\n", (unsigned)difftime(time(NULL), curr));
	static_arr_info winstates = init_sarr(0,1);
	winstates.bp[0] = 0x0000000007ff0ff;
	log_out("Benchmarking multi-threaded solving (LL-128)", LOG_INFO_);
	curr = time(NULL);
	set_log_level(LOG_ERROR_);
	solve(150, 16, "/tmp/cablegen/%d.boards", "/tmp/cablegen/%d.tables", &winstates, get_settings().cores, 0, 0, 0);
	set_log_level(LOG_INFO_);
	printf("Multi-threaded LL-128: %d seconds\n", (unsigned)difftime(time(NULL), curr));
}

uint64_t factorial(uint16_t inp){
	if(inp == 0)
		return 1;
	uint64_t res = inp;
	for(int i = 1; i < inp; i++)
		res *= i;
	return res;
}

void print_binary(uint16_t num) {
    for (int i = sizeof(uint16_t) * 8 - 1; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
    }
}

int main(int argc, char **argv){
	set_log_level(LOG_INFO_);
	init_settings();
	// parse arguments
	if(argc > 1){
		if(!strcmp(argv[1],"help")) {help();}
		else if(!strcmp(strlwr_(argv[1]), "generate")) {parseGenerate(argc, argv);}
		else if(!strcmp(strlwr_(argv[1]), "solve")) {parseSolve(argc, argv);}
		else if(!strcmp(strlwr_(argv[1]), "test")) {test();}
		else if(!strcmp(strlwr_(argv[1]), "read")) {parseRead(argc, argv);}
		else if(!strcmp(strlwr_(argv[1]), "write")) {parseWrite(argc, argv);}
		else if(!strcmp(strlwr_(argv[1]), "lookup")) {parseLookup(argc, argv, false);}
		else if(!strcmp(strlwr_(argv[1]), "lookup_spawn")) {parseLookup(argc, argv, true);}
		else if(!strcmp(strlwr_(argv[1]), "explore")) {parseExplore(argc, argv);}
		else if(!strcmp(strlwr_(argv[1]), "play")) {parsePlay(argc, argv);}
		else if(!strcmp(strlwr_(argv[1]), "train")) {parseTrain(argc, argv);}
		else if(!strcmp(strlwr_(argv[1]), "benchmark")) {benchmark();}
		else{
			log_out("Unrecognized command!", LOG_WARN_);
			help();
		}
	}
	else{
		help();
	}
	exit(0);
	return 0;
}
