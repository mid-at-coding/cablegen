#ifndef PARSE_H
#include <stddef.h>

typedef struct {
	int (*callback)(char*, void*);
	char *short_rep;
	char *long_rep;
	void *data;
	size_t eq;
} option_t;

int parse(option_t *, size_t, int, char **);

int parseBool  (char*, void*);
int parseInt   (char*, void*);
int parseLong  (char*, void*);
int parseLL    (char*, void*);
int parseDouble(char*, void*);
int parseHex   (char*, void*);
int parseStr   (char*, void*);
#endif
