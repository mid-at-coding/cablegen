#include "format.h"
#include "array.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

char *format_str(const char *fmt, ...){
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

int val(char input){
	if(input >= '0' && input <= '9')
		return input - '0';
	if(isalpha(input))
		return tolower(input) - 'a' + 10;
	return -1;
}

bool isdigit_radix(const char input, const unsigned int radix){
	int res = val(input);
	if(res == -1)
		return false;
	return (unsigned)res < radix;
}

atoi_res safe_atoi(const char *input){ // you could make this a thunk of safe_atoi_ll but it's a pita
	atoi_res res = {0};
	errno = 0; // strtol need not set errno to 0 on a sucessful call, so zero-initialize errno
	res.res = strtol(input, NULL, 10); // use strtol because atoi need not set errno on failure
	if(res.res == LONG_MAX  || res.res == LONG_MIN){ // in this case, a conforming implementation must set errno to ERANGE,
							 // but check anyway
		res.status = ERANGE;
	}
	else if(errno){ // technically a conforming implementation may only set errno to ERANGE, but check for any error
		res.status = errno;
		errno = 0;
	}
	else if(!res.res){ // either our number is zero, or our input is whitespace followed by a non-digit number
		// check if the first non space character in input isn't a number
		for(const char *c = input; *c; c++){
			if(isspace(*c)) // the leading whitespace must be specified by isspace (7.22.1.4 #2)
				continue;
			if(!isdigit(*c)) // in the case of radix == 10, this works
				res.status = EINVAL;
		}
	}
	return res;
}

atoi_res_ll safe_atoi_ll(const char *input, const int radix){
	atoi_res_ll res = {0};
	if(radix < 2 || radix > 36){ // UB
		res.status = EINVAL;
		return res;
	}
	errno = 0; // strtol need not set errno to 0 on a sucessful call, so zero-initialize errno
	res.res = strtoll(input, NULL, radix); // use strtoll because atoll need not set errno on failure
	if(res.res == LLONG_MAX  || res.res == LLONG_MIN){ // in this case, a conforming implementation must set errno to ERANGE, but check anyway
		res.status = ERANGE;
	}
	else if(errno){ // technically a conforming implementation may only set errno to ERANGE, but check for any error
		res.status = errno;
		errno = 0;
	}
	else if(!res.res){ // either our number is zero, or our input is whitespace followed by a non-digit number
		// check if the first non space character in input isn't a number
		for(const char *c = input; *c; c++){
			if(isspace(*c)) // the leading whitespace must be specified by isspace (7.22.1.4 #2)
				continue;
			if(!isdigit_radix(*c, radix)) // custom function that's written to specs defined in 7.22.1.4 #3
				res.status = EINVAL;
		}
	}
	return res;
}

