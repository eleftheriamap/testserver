#include "wrapper/io.h"
#include <stdlib.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdbool.h>

#include "utils/lib.h"
#include "utils/log.h"

static log_config config ;

bool log_labels() {
    return config.log_labels ;
}

#define LOG_WRAP(res, format) \
    va_list args; \
    va_start( args, format ); \
    int res = vlog_fmt( format, args ); \
    va_end( args )

void set_log_output(log_type_e e) {
    config.output = e ;
}

void log_error_handler(jmp_buf b, int jmp_val, const char *format, ...) {
    va_list args; 
    va_start( args, format ); 
    int res = vlog_fmt( format, args ); 
    va_end( args ) ;
    longjmp(b, jmp_val) ;
}

void set_log_level(log_level l) {
    config.level = l ;
}

int vlog_fmt(const char *format, va_list args) {
    int res = 0 ;
    // if (log_type == LOG_STDOUT) 
    res |= vfprintf(stdout, format, args) ;
    res |= fflush(stdout) ;
    return 0;
}

int loglvl(log_level l, const char *format, ...) {
    if (l < config.level) return 0;
    LOG_WRAP(res, format) ;
    return res;
}

int log_fmt(const char *format, ...) {
    LOG_WRAP(res, format) ;
    return res;
}

int logln(const char *format, ...) {
    LOG_WRAP(res, format) ;
    res |= log_fmt("\n") ;
    return res ;
}

int log_barline() {
    return logln("---------------------------------------------------------------------") ;
}

static char* cur_stage ;

/// @brief Output a log indicating a major 'stage'.
/// @param s string name of the stage 
int log_stage(const char* s) {
    if (!config.log_stages) return 0 ;
    asprintf(&cur_stage, "%s", s) ;
    int res = log_barline() ;
    res |= logln("Stage: %s", s) ;
    return res ;    
} 

/// Outputs end of current stage. Outputs error if no current stage.
int log_end_stage() {
    if (cur_stage == NULL) {
        return logln("Finished Stage") ;
        // logln("! Tried to finish stage, but `cur_stage` was null") ;
        // return 0 ;
    }
    return logln("Finished Stage: %s", cur_stage) ;
}

int log_error(const char *format, ...) {
    log_fmt("\033[0;31m" "! INTERNAL ERROR: " "\033[0m") ;
    LOG_WRAP(res, format) ;
    logln("") ;
    exit(1) ;
    return res;
}


// log_fmt("\033[0;32m" "SUCCESS: " "\033[0m") ; logln(__VA_ARGS__) ;

// int log_success(const char *format, ...) {
//     log_fmt("\033[0;32m" "SUCCESS: " "\033[0m") ;
//     LOG_WRAP(res, format) ;
//     logln("") ;
//     return res;
// }

void log_exit_failure(const char *format, ...) {
    LOG_WRAP(res, format) ;
    logln("") ;
    exit(EXIT_FAILURE);
}

void log_exit_code(int __status, const char *format, ...) {
    LOG_WRAP(res, format) ;
    logln("") ;
    exit(__status);
}

int log_init(const char *msg, ...) {
    if (!config.log_inits) return 0 ;
    LOG_WRAP(res, msg) ;
    return res ;
}

int log_IO(const char *msg, ...) {
    if (!config.log_IO) return 0 ;
    LOG_WRAP(res, msg) ;
    return res ;
}

int log_DEC(const char *msg, ...) {
    if (!config.log_DEC) return 0 ;
    LOG_WRAP(res, msg) ;
    return res ;
}


void setup_log() {
    set_log_level(LOG_0);
    set_log_output(LOG_STDOUT);
    config.log_inits = true ;
    config.log_stages = true ;
    config.log_labels = true ;
    config.log_IO = true ;
    config.log_free = true ;
}

void set_config(log_config c) {
    config = c ;
}

void set_config_debug() {
    config = DEBUG_LOG ;
}

void set_config_std() {
    config = STD_LOG ;
}

void __log_free(void *__ptr, const char *format, ...) {
    if (!config.log_free) return ;
    LOG_WRAP(res, format) ;
    free(__ptr) ;
}

bool log_level_enabled(log_level l) {
    return l >= config.level ;
}
