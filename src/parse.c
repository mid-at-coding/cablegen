#include "parse.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static bool prefix(const char *pre, const char *str){ // TODO make sure the prefix matches entirely until an eq
    return strncmp(pre, str, strlen(pre));
}

static size_t find_eq(const char *str){
	for(const char *curr = str; *curr != '\0'; curr++){
		if(*curr == '=')
			return curr - str;
	}
	return 0;
}

static int parse_opt(option_t *opt, char *arg){
	if(prefix(opt->short_rep, arg) && prefix(opt->long_rep, arg))
		return 1;
	int res = opt->callback(arg, opt);
	if(res < 0)
		return -1;
	return 0;
}

int parse(option_t *opts, size_t len_opts, int argc, char **argv){
	int res = 0;
	bool arg_parsed;
	for(char **curr_arg = argv + 1; curr_arg < argv + argc; curr_arg++){ // + 1 to skip over the program
		arg_parsed = false;
		for(option_t *opt = opts; opt < opts + len_opts; opt++){
			res = parse_opt(opt, *curr_arg);
			if(res < 0)
				return res; // assume that the callback will print its own diagnostic
			else if(!res){
				arg_parsed = true;
				break;
			}
		}
		if(!arg_parsed){
			printf("Unknown argument %s\n", *curr_arg);
			return -1;
		}
	}
	return 0;
}

static size_t get_eq(char *arg, char *name, char *type){ // TODO: combine with prefix, maybe pull out into parse_opt
	size_t eq = find_eq(arg);
	if(!eq){
		printf("Invalid flag! Do %s=%s\n", name, type);
		return 0;
	}
	return eq;
}

int parseBool(char* arg, void* data){
	char *t[] = {
		"true",
		"t",
		"1"
	};
	char *f[] = {
		"false",
		"f",
		"0"
	};
	option_t *opt = data;
	size_t eq = get_eq(arg, opt->long_rep, "bool");
	if(!eq)
		return -1;
	eq += 1; // just past the equals
	for(size_t i = 0; i < sizeof(t)/sizeof(t[0]); i++){
		if(!strcmp(t[i], arg + eq)){
			*((bool*)opt->data) = true;
			return 0;
		}
		if(!strcmp(f[i], arg + eq)){
			*((bool*)opt->data) = false;
			return 0;
		}
	}
	printf("Invalid value %s for boolean flag %s!\n", arg + eq, opt->long_rep);
	return -1;
}
int parseInt(char* arg, void* data){
	option_t *opt = data;
	size_t eq = get_eq(arg, opt->long_rep, "int");
	if(!eq)
		return -1;
	errno = 0;
	*(int*)opt->data = (int)strtol(arg + eq + 1, NULL, 10);
	if(errno){
		printf("Invalid value %s for integer flag!\n", arg + eq); 
		return -1;
	}
	return 0;
}
int parseLong(char* arg, void* data){
	option_t *opt = data;
	size_t eq = get_eq(arg, opt->long_rep, "long");
	if(!eq)
		return -1;
	errno = 0;
	*(long*)opt->data = strtol(arg + eq + 1, NULL, 10);
	if(errno){
		printf("Invalid value %s for long flag!\n", arg + eq); 
		return -1;
	}
	return 0;
}
int parseLL(char* arg, void* data){
	option_t *opt = data;
	size_t eq = get_eq(arg, opt->long_rep, "long");
	if(!eq)
		return -1;
	errno = 0;
	*(long*)opt->data = strtoll(arg + eq + 1, NULL, 10);
	if(errno){
		printf("Invalid value %s for long flag!\n", arg + eq); 
		return -1;
	}
	return 0;
}
int parseDouble(char* arg, void* data){
	option_t *opt = data;
	size_t eq = get_eq(arg, opt->long_rep, "double");
	if(!eq)
		return -1;
	errno = 0;
	*(double*)opt->data = strtod(arg + eq + 1, NULL);
	if(errno){
		printf("Invalid value %s for double flag!\n", arg + eq); 
		return -1;
	}
	return 0;
}
int parseHex(char* arg, void* data){
	option_t *opt = data;
	size_t eq = get_eq(arg, opt->long_rep, "0xD34DBEEF");
	if(!eq)
		return -1;
	errno = 0;
	*(unsigned long long*)opt->data = strtoull(arg + eq + 1, NULL, 16);
	if(errno){
		printf("Invalid value %s for hex flag!\n", arg + eq); 
		return -1;
	}
	return 0;
}
int parseStr(char* arg, void* data){
	option_t *opt = data;
	size_t eq = get_eq(arg, opt->long_rep, "string");
	if(!eq)
		return -1;
	char *buf = malloc(strlen(arg) - eq);
	if(!buf){
		printf("Could not allocate string buffer for argument!\n");
		return -1;
	}
	strcpy(buf, arg + eq + 1);
	*(char**)opt->data = buf;
	return 0;
}
