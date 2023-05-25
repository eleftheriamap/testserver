#ifndef __UTILS_STRING_FUNCS_H
#define __UTILS_STRING_FUNCS_H

#include <stdbool.h>

extern bool is_whitespace(char *) ;

extern bool streq(const char *s1, const char *s2) ;
extern bool prefix(char *pre, char *str) ;
extern void strtolower(char *s) ;

#endif
