#include "utils/file.h"
#include "utils/log.h"

/// @brief Open file, log an error if open fails.
/// @param fname string path of file name
/// @param modes modes to open file with
/// @param reason string reason to log on failure.
/// @return opened file, possibly null.
FILE *s_fopen(char *fname, char *modes, const char *reason) {
    FILE *f_ptr = fopen(fname, modes);
    if (f_ptr == NULL)
        log_error("Error: failed to open file for %s: '%s'\n", reason, f_ptr);
    return f_ptr;
}
