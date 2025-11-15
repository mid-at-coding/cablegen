#ifndef SETTINGS_H
#define SETTINGS_H
#include <stdbool.h>
#include "cfgpath.h"
extern bool settings_read;
extern bool custom_path;
extern char cfgdir[MAX_PATH];
char *strlwr_(char *str);
void change_config(char *cfg);
typedef struct {
	bool free_formation;
	bool ignore_f;
	long long cores;
	long long nox;
	bool mask;
	bool compress;
	long long max_prealloc;

	bool premove;
	char *bdir;
	char *initial;
	long long end_gen;
	long long stsl;
	long long ltc;
	long long smallest_large;
	bool prune;

	char *tdir;
	char *winstates;
	long long end_solve;
	bool score;
	bool delete_boards;
} settings_t;

settings_t *get_settings(void);
void init_settings(void);

#endif
