#include "../inc/settings.h"
#include "../inc/cfgpath.h"
#include "../inc/ini.h"
#include "../inc/logging.h"
#include "../inc/array.h"
#include <errno.h>
#include <string.h>
#include <ctype.h>
char *strlwr_(char *str) {
  unsigned char *p = (unsigned char *)str;
  while (*p) {
     *p = tolower((unsigned char)*p);
      p++;
  }
  return str;
}
static char cfgdir[MAX_PATH];
#define MAX_PROP_SIZE 100
static ini_t* get_cfg(){
	static bool init = false;
	if(!init){
		get_user_config_file(cfgdir, sizeof(cfgdir), "cablegen");
		if (cfgdir[0] == 0) {
			log_out("Could not find config directory!", LOG_WARN_);
			return NULL;
		}
	}
	return ini_load(cfgdir);
}
void change_config(char *cfg){	
	if(cfg == NULL || strlen(cfg) > MAX_PATH){
		log_out("Invalid config file location!", LOG_WARN_);
		return;
	}
	ini_free(get_cfg()); // make sure it's already initialized
	strcpy(cfgdir, cfg);
}
int get_str_setting(char *key, char **str){
	int e = get_str_setting_section(key, "Cablegen", str);
	if(e)
		return e;
	return 0;
}
int get_str_setting_section (char *key, char *section, char** str){
	auto cfg = get_cfg();
	if(cfg == NULL){
		ini_free(cfg);
		return 1;
	}
	char *res = ini_get(cfg, section, key);
	if(res == NULL){
		log_out("Could not find property!", LOG_WARN_);
		ini_free(cfg);
		return 1;
	}
	*str = res;
	ini_free(cfg);
	return 0;
}
int get_bool_setting(char *key, bool *data){
	char *str = malloc_errcheck(MAX_PROP_SIZE);
	int e = get_str_setting(key, &str);
	if(e){
		return e;
	}
	strlwr_(str);
	*data = !strcmp(str, "true");
	return 0;
}
long long get_int_setting (char *key, int *data){
	char *str = malloc_errcheck(MAX_PROP_SIZE);
	int e = get_str_setting(key, &str);
	if(e){
		*data = 0;
		return e;
	}
	*data = atoll(str);
	return 0;
}

int get_bool_setting_section(char *key, char *section, bool *data){
	char *str = malloc_errcheck(MAX_PROP_SIZE);
	int e = get_str_setting_section(key, section, &str);
	if(e){
		return e;
	}
	strlwr_(str);
	*data = !strcmp(str, "true");
	return 0;
}
long long get_int_setting_section (char *key, char *section, int *data){
	char *str = malloc_errcheck(MAX_PROP_SIZE);
	int e = get_str_setting_section(key, section, &str);
	if(e){
		return e;
	}
	*data = atoll(str);
	return 0;
}
