#ifndef FORMAT_H
#define FORMAT_H
typedef struct {
	long res;
	int status;
} atoi_res;
typedef struct {
	long res;
	int status;
} atoi_res_ll;
char *format_str(const char *fmt, ...);
atoi_res safe_atoi(const char *input);
atoi_res_ll safe_atoi_ll(const char *input, const int radix);
bool isdigit_radix(const char input, const unsigned int radix);
#endif
