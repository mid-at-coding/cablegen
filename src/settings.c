#include "../inc/settings.h"
#include "../inc/cfgpath.h"
#include "../inc/ini.h"
#include "../inc/logging.h"
#include "../inc/array.h"
#include <string.h>
#include <ctype.h>
#ifdef WINDOWS
#include <direct.h>
#define getcwd_ _getcwd
#else
#include <unistd.h>
#define getcwd_ getcwd
#endif
bool settings_read = false;
bool custom_path = false;
static int get_bool_setting(const char *key, bool*);
static int get_str_setting (const char *key, char**);
static int get_int_setting (const char *key, long long*);

static int get_bool_setting_section(const char *key, char *section, bool*);
static int get_str_setting_section (const char *key, char *section, char**);
static int get_int_setting_section (const char *key, char *section, long long*);

settings_t get_settings(void){
	static settings_t res = { // set sane defaults
		.free_formation = 0,
		.ignore_f = 0,
		.cores = 1,
		.nox = 0,
		.mask = 0,
		
		.premove = false,
		.bdir = "./boards/",
		.initial = "./initial",
		.end_gen = 1200,
		.stsl = 200,
		.ltc = 5,
		.smallest_large = 6,
		.prune = false,

		.tdir = "./tables/",
		.winstates = "./winstates",
		.end_solve = 0,
		.score = false
	};
	if(settings_read)
		return res;
	settings_read = true; 
	log_out("Reading settings...", LOG_DBG_);
	get_bool_setting("free_formation", &res.free_formation);
	get_bool_setting("ignore_f", &res.ignore_f);
	get_int_setting("cores", &res.cores); 
	get_int_setting("nox", &res.nox); 
	get_bool_setting("mask", &res.mask); 
	get_bool_setting_section("premove", "Generate", &res.premove);
	get_str_setting_section("dir", "Generate", &res.bdir);
	get_str_setting_section("initial", "Generate", &res.initial);
	get_int_setting_section("end", "Generate", &res.end_gen); 
	get_int_setting_section("stsl", "Generate", &res.stsl); 
	get_int_setting_section("ltc", "Generate", &res.ltc); 
	get_int_setting_section("smallest_large", "Generate", &res.smallest_large); 
	get_bool_setting_section("prune", "Generate", &res.prune);
	get_str_setting_section("dir", "Solve", &res.tdir);
	get_str_setting_section("winstates", "Solve", &res.winstates);
	get_int_setting_section("end", "Solve", &res.end_solve); 
	get_bool_setting_section("score", "Solve", &res.score);
	return res;
}

char *strlwr_(char *str) {
  unsigned char *p = (unsigned char *)str;
  while (*p) {
     *p = tolower((unsigned char)*p);
      p++;
  }
  return str;
}
char cfgdir[MAX_PATH];
#define MAX_PROP_SIZE 100
ini_t* get_cfg(void){
	if(!custom_path){
		get_user_config_file(cfgdir, sizeof(cfgdir), "cablegen");
	}
	if (cfgdir[0] == 0) {
		log_out("Could not find config directory!", LOG_WARN_);
		return NULL;
	}
	log_out("Loading config from: ", LOG_DBG_);
	log_out(cfgdir, LOG_DBG_);
	return ini_load(cfgdir);
}
void change_config(char *cfg){	
	if(cfg == NULL || strlen(cfg) >= MAX_PATH){
		log_out("Invalid config file location!", LOG_WARN_);
		return;
	}
	settings_read = false; // update settings reading
	custom_path = true;
	memcpy(cfgdir, cfg, strlen(cfg) + 1);
	log_out("New config dir: ", LOG_DBG_);
	log_out(cfgdir, LOG_DBG_);
}
void init_settings(void){
	char buf[MAX_PATH];
	getcwd_(buf, MAX_PATH);
	if(strlen(buf) + strlen("cablegen.conf") >= MAX_PATH || errno){
		log_out("Invalid config file location!", LOG_WARN_);
		return;
	}
	log_out("Reading settings from conf dir", LOG_TRACE_);
	get_settings();
	strcat(buf, "/cablegen.conf");
	log_out("Reading settings from local dir", LOG_TRACE_);
	log_out(buf, LOG_TRACE_);
	change_config(buf);
	get_settings();
}
static int get_str_setting(const char *key, char **str){
	int e = get_str_setting_section(key, "Cablegen", str);
	if(e)
		return e;
	return 0;
}
static int get_str_setting_section (const char *key, char *section, char** str){
	ini_t* cfg = get_cfg();
	if(cfg == NULL){
		return 1;
	}
	char *res = ini_get(cfg, section, key);
	if(res == NULL){
		log_out("Could not find property!", LOG_DBG_);
		log_out(key, LOG_DBG_);
		ini_free(cfg);
		return 1;
	}
	char *rres = malloc_errcheck(strlen(res) + 1);
	strcpy(rres, res);
	*str = rres;
	ini_free(cfg);
	return 0;
}
static int get_bool_setting(const char *key, bool *data){
	char *str;
	int e = get_str_setting(key, &str);
	if(e){
		return e;
	}
	strlwr_(str);
	*data = !strcmp(str, "true");
	free(str);
	return 0;
}
static int get_int_setting (const char *key, long long *data){
	char *str;
	int e = get_str_setting(key, &str);
	if(e){
		return e;
	}
	*data = atoll(str);
	free(str);
	return 0;
}

static int get_bool_setting_section(const char *key, char *section, bool *data){
	char *str;
	int e = get_str_setting_section(key, section, &str);
	if(e){
		return e;
	}
	strlwr_(str);
	*data = !strcmp(str, "true");
	free(str);
	return 0;
}
static int get_int_setting_section (const char *key, char *section, long long *data){
	char *str;
	int e = get_str_setting_section(key, section, &str);
	if(e){
		return e;
	}
	*data = atoll(str);
	free(str);
	return 0;
}
