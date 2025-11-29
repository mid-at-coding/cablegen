#include "format.h"
#include "array.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

char *format_str(char *fmt, ...){
	va_list v, v1;
	va_start(v, fmt);
	va_copy(v1, v);
	size_t size = vsnprintf(NULL, 0, fmt, v);
	va_end(v);
	char *buf = malloc_errcheck(size + 1);
	vsprintf(buf, fmt, v1);
	va_end(v1);
	return buf;
}
