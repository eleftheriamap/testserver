#include <string.h>
#include <ctype.h>

#include "utils/string_funcs.h"


/// @brief True when two strings are equal.
inline
bool streq(const char *s1, const char *s2) {
    return !strcmp(s1, s2);
}

/// @brief True when the first string is a prefix of the second.
inline
bool prefix(char *pre, char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

/**
 * @brief Convert a string to lower case (inplace).
 * This function edits the original string.
 * 
 * @param s The string to be converted.
 */
inline
void strtolower(char *s) {
    for(int i = 0; s[i]; i++) {
        s[i] = tolower(s[i]) ;
    }
}

inline
bool is_whitespace(char *line) {
    for (int i = 0; line[i]; i++) {
        if (!isspace(line[i])) return false ;
    }
    return true ;
}
