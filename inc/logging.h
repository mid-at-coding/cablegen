#ifndef LOGGING_H
#define LOGGING_H
#include <string.h>

enum LOG_LEVEL{
	LOG_TRACE_,
	LOG_DBG_,
	LOG_INFO_,
	LOG_WARN_,
	LOG_ERROR_
};

void log_out(const char* str, enum LOG_LEVEL level);
void set_log_level(enum LOG_LEVEL level);

#endif
