#include <raylib.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include "../inc/test.h"
#include "../inc/logging.h"
#include "../inc/generate.h"
#include "../inc/board.h"
#include "../inc/settings.h"

static int numPlaces (int n) {
    if (n < 0) return numPlaces ((n == INT_MIN) ? INT_MAX: -n) + 1;
    if (n < 10) return 1;
    return 1 + numPlaces (n / 10);
}

static void help(){
	log_out("Cablegen v0.0 by ember/emelia/cattodoameow", LOG_INFO_);
	log_out("Commands:", LOG_INFO_);
	log_out("help -- this output", LOG_INFO_);
	log_out("generate [FILE] [DIR] [END] -- generate all sub-boards of the boards in FILE in DIR until the tile sum meets END", LOG_INFO_);
	log_out("solve [BDIR] [TDIR] [START] [END] -- solve all boards in BDIR backwards, putting the solutions in TDIR", LOG_INFO_);
}

static void parseGenerate(int argc, char **argc){
	if(argc < 5){ log_out("Not enough arguments!", LOG_ERROR_); }
}

static void parseSolve(int argc, char **argc){
	if(argc < 6){ log_out("Not enough arguments!", LOG_ERROR_); }
}

int main(int argc, char **argv){
	// parse arguments
	// valid commands:
	// 	help
	// 	generate [board file] [dir] [end]
	// 	solve [board dir] [table dir] [start] [end]
	if(argc > 1){
		if(argv[1] == "help") {help();}
		else if(argv[1] == "generate") {parseGenerate(argc, argv);}
		else if(argv[1] == "solve") {parseSolve(argc, argv);}
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
