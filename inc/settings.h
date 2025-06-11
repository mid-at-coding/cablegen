#ifndef SETTINGS_H
#define SETTINGS_H
#include <errno.h>
#include <stdbool.h>
char *strlwr_(char *str);
void change_config(char *cfg);
typedef struct {
	bool free_formation;
	long long cores;
	long long nox;

	bool premove;
	char *bdir;
	char *initial;
	long long end_gen;

	char *tdir;
	char *winstates;
	long long end_solve;
	bool score;
} settings_t;

settings_t get_settings(void);

#endif
