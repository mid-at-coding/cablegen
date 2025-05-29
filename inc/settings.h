#ifndef SETTINGS_H
#define SETTINGS_H
#include <errno.h>
#include <stdbool.h>
void change_config(char *cfg);
int get_bool_setting(char *key, bool*);
int get_str_setting (char *key, char**);
long long get_int_setting (char *key, int*);

int 	  get_bool_setting_section(char *key, char *section, bool*);
int 	  get_str_setting_section (char *key, char *section, char**);
long long get_int_setting_section (char *key, char *section, int*);

char *strlwr_(char *);
#endif
