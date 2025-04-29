#include "../inc/settings.h"
#include "../inc/cfgpath.h"
#include "../inc/ini_file.h"
#include "../inc/logging.h"
#include "../inc/array.h"
#include <errno.h>
#include <string.h>
#include <ctype.h>
static char *strlwr(char *str) {
  unsigned char *p = (unsigned char *)str;
  while (*p) {
     *p = tolower((unsigned char)*p);
      p++;
  }
  return str;
}
#define MAX_PROP_SIZE 100
static int get_cfg(Ini_File *file){
	// get config file path
	char cfg_file[MAX_PATH];
	get_user_config_file(cfg_file, MAX_PATH, "cablegen");
	if(!strcmp(cfg_file, "")){ // we have no config dir
		log_out("Could not find config dir!", LOG_WARN_);
		return ENOENT;
	}
	file = ini_file_parse(cfg_file, NULL);
	if(file == NULL){
		log_out("Could not parse config file, trying to create one.", LOG_WARN_);
		file = ini_file_new();
		Ini_File_Error e = ini_file_add_section(file, "Cablegen");
		if(e != 0)
			log_out("Failed to create config section!", LOG_WARN_);
		e = ini_file_save(file, cfg_file);
		if(e != 0){
			log_out("Failed to create config file!", LOG_WARN_);
			return ENOENT;
		}
	}
	return 0;
}
int get_bool_setting(char *key, bool* data){
	char *str = malloc_errcheck(MAX_PROP_SIZE);
	int e;
	if((e = get_str_setting(key, &(str))))
		return e;
	if(!strcmp(strlwr(str), "true"))
		*data = true;
	return 0;
}
int get_str_setting (char *key, char** str){
	Ini_File *file = malloc_errcheck(sizeof(Ini_File));
	int e;
	if((e = get_cfg(file)))
		return e;
	e = ini_file_find_property(file, "Cablegen", key, str);
	if(e != 0){ // assume this is due to the prop not existing
		log_out("Could not find property!", LOG_WARN_);
		return EFAULT;
	}
	return 0;
}
int get_int_setting (char *key, int* data){
	char *str = malloc_errcheck(MAX_PROP_SIZE);
	int e;
	if((e = get_str_setting(key, &str)))
		return e;
	*data = atoi(str);
	return 0;
}

int set_bool_setting(char *key, bool data){
	return set_str_setting(key, data ? "true" : "false");
}
int set_str_setting (char *key, char* data){
	Ini_File *file = malloc_errcheck(sizeof(Ini_File));
	int e = get_cfg(file);
	if(e)
		return e;
	Ini_File_Error err = ini_file_add_property(file, key, data);
	if(err)
		return err;
	char cfg_file[MAX_PATH];
	get_user_config_file(cfg_file, MAX_PATH, "cablegen");
	if(!strcmp(cfg_file, "")){ // we have no config dir
		log_out("Could not find config dir!", LOG_WARN_);
		return ENOENT;
	}
	return ini_file_save(file, cfg_file);
}
int set_int_setting (char *key, int data){
	char str[MAX_PROP_SIZE];
	snprintf(str, MAX_PROP_SIZE, "%d", data);
	return set_str_setting(key, str);
}
