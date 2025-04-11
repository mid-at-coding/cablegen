#ifndef SETTINGS_H
#define SETTINGS_H
#include <errno.h>
#include <stdbool.h>
int get_bool_setting(char *key, bool*);
int get_str_setting (char *key, char**);
int get_int_setting (char *key, int*);

int set_bool_setting(char *key, bool);
int set_str_setting (char *key, char*);
int set_int_setting (char *key, int);
#endif
