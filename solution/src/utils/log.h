#ifndef __LOG_H
#define __LOG_H

#define TODO exit(1)

#include <stdbool.h>
#include <setjmp.h>
#include <stdarg.h>

typedef enum log_type {
    LOG_NONE,
    LOG_STDOUT,
} log_type_e ;

typedef enum log_level {
    LOG_0,
    LOG_1,
    LOG_2,
    LOG_3,
    LOG_4,
    LOG_ERROR,
} log_level ;

typedef struct log_config {
    log_level level ;
    log_type_e output ;
    bool log_inits ;
    bool log_stages ;
    bool log_labels ;
    bool log_IO ;
    bool log_DEC ;
    bool log_free ;
}   log_config ;

#define DEBUG_LOG (struct log_config) \
    {.level=LOG_0, .output=LOG_STDOUT, \
    .log_inits=true, .log_stages=true, \
    .log_labels=true, .log_IO=true, .log_DEC=true, .log_free=true}

#define STD_LOG (struct log_config) \
    {.level=LOG_3, .output=LOG_STDOUT, \
    .log_inits=false, .log_stages=false,\
    .log_labels=false, .log_IO=false, .log_DEC=false, .log_free=false}
    

#define ASSERT(b) do {if (!(b)) {log_error("Assertion failed: %s", #b) ; exit(EXIT_FAILURE) ;}} while (0)
#define ASSERT_M_OLD(b, s) do {if (!(b)) {log_error(s); exit(EXIT_FAILURE);}} while (0)
#define ASSERT_M(b, msg...) do {if (!(b)) {log_error(msg) ; exit(EXIT_FAILURE);};} while(0)
// #define ASSERT_(b, msg, ...) do {if (!(b)) {log_error(msg,__VA_ARGS__) ; exit(EXIT_FAILURE);};} while(0)
#define FAIL_M(e) do { log_error(e); exit(EXIT_FAILURE);} while (0)

void set_log_output(log_type_e e);
void set_log_level(log_level l);

void log_error_handler(jmp_buf b, int jmp_val, const char *format, ...) ;

void log_exit_failure(const char *, ...) __attribute__ ((__noreturn__)) ;
int loglvl(log_level l, const char *, ...) ;
int log_fmt(const char *, ...) ;
int logln(const char *, ...) ;
int log_error(const char *, ...) ;
// int log_success(const char *format, ...) ;
// int not_implemented_error(const char *format, ...) ;
#define not_implemented_error(msg...) \
    log_fmt("NOT IMPLEMENTED - %s: ", __func__) ; \
    log_exit_failure(msg) ; \
    exit(EXIT_FAILURE)
#define log_success(format, msg...) logln("\033[0;32m" "SUCCESS: " "\033[0m" format, msg) 
int log_barline() ;
int log_stage(const char*) ;
int log_end_stage() ;

bool log_labels() ;

int log_init(const char *, ...) ;
int log_IO(const char *, ...) ;
int log_DEC(const char *, ...) ;

int vlog_fmt(const char *format, va_list args) ;

void setup_log() ;
void set_config(log_config c) ;
void set_config_debug() ;
void set_config_std() ;

#define log_free(ptr) __log_free(ptr, "In %s: Freeing %s", __func__, #ptr)
#define logf_free(ptr, format, args...) __log_free(ptr, "In %s: Freeing %s" format, __func__, #ptr, ##args)
void __log_free(void *__ptr, const char *format, ...) ;

bool log_level_enabled(log_level) ;

#endif
