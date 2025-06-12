#include "../inc/logging.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum LOG_LEVEL sensitivity;

static char* get_prefix(enum LOG_LEVEL level){
	switch (level){
		case LOG_TRACE_:
			return "[TRACE]";
			break;
		case LOG_DBG_:
			return "[DEBUG]";
			break;
		case LOG_INFO_:
			return "[INFO]";
		case LOG_WARN_:
#ifdef _WIN32
			return "[WARN]";
#else
			return "\033[93m[WARN]";
#endif
		case LOG_ERROR_:
#ifdef _WIN32
			return "[ERROR]";
#else
			return "\033[91m[ERROR]";
#endif
	}
	return "";
}

static char* get_postfix(enum LOG_LEVEL level){
	if(level == LOG_ERROR_ || level == LOG_WARN_){
		return "\033[39m";
	}
	return "";
}

bool logif_f(enum LOG_LEVEL level){ return level >= sensitivity; }

void log_out(const char* strin, enum LOG_LEVEL level){
	char* str = malloc(strlen(strin) + 1);
	if(str == NULL)
		return; // would print a diagnostic but...
	strcpy(str, strin);
	if(level >= sensitivity){
		if(str[strlen(str) - 1] == '\n'){
			str[strlen(str) - 1] = ' ';
		}
		printf("%s %s %s\n", get_prefix(level), str, get_postfix(level));
	}
	free(str);
}
void set_log_level(enum LOG_LEVEL level){
	sensitivity = level;
}



