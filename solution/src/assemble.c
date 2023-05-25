#include <stdlib.h>


#include "utils/log.h"
#include "assembler/assembler.h"
#include "utils/file.h"
#include "utils/string_funcs.h"

static log_config mk_config() {
    log_config c = STD_LOG ;
    c.log_inits = false ;
    return c ;
}

typedef struct arg_config {
    char *src ;
    char *dst ;
    bool has_listing ;
    char *listdst ;
    bool help ;
    bool verbose ;
}   arg_config ;

static const char *options = "[-(v|h)] <assembler> <binary> [<listing>]";
static const char *help = 
    "  -v: verbose mode\n"
    "  -h: help (print this)\n"
    "  <assembler>: the file containing the assembly code to assemble\n"
    "  <binary>: the file to write the assembled binary to\n"
    "  <listing>: the file to write the listing to\n" ; 

void parse_arg(int argc, char **args, int *argi, arg_config *cfg) {
    char *arg = args[*argi] ;
    if (arg[0] == '-') {
        if (arg[1] == 'v') {
            cfg->verbose = true ;
        } else if (arg[1] == 'h') {
            cfg->help = true ;
        } else {
            log_exit_failure("Unknown argument %s\n", arg) ;
        }
    } else {
        if (cfg->src == NULL) {
            cfg->src = arg ;
        } else if (cfg->dst == NULL) {
            cfg->dst = arg ;
        } else if (cfg->listdst == NULL) {
            cfg->listdst = arg ;
        } else {
            log_exit_failure("Too many arguments\n") ;
        }
    }
}


void parse_args(int argc, char **argv, arg_config *cfg) {
    if (argc < 3)
        log_exit_failure("Usage: %s %s\n", argv[0], options);

    for (int i = 1; i < argc; i++) {
        parse_arg(argc, argv, &i, cfg) ;
    }
}

int main(int argc, char **argv) {
    setup_log() ;
    set_config(mk_config()) ;

    arg_config cfg = {} ;
    parse_args(argc, argv, &cfg) ;

    if (cfg.help) {
        printf("Usage: %s %s\n", argv[0], options) ;
        printf("%s", help) ;
        exit(EXIT_SUCCESS) ;
    }

    if (cfg.verbose) { set_config_debug() ; } 
    else { set_config_std() ; }

    FILE *in = s_fopen(cfg.src, "r", "reading") ;
    FILE *out = s_fopen(cfg.dst, "wb", "writing");
    FILE *listing = NULL ;
    if (cfg.has_listing) { listing = s_fopen(cfg.listdst, "w", "listing") ; }
    
    log_IO("Assembling %s to %s", cfg.src, cfg.dst) ;
    if (cfg.has_listing) { log_IO("Listing %s", cfg.listdst) ; }
    int res = assemble_listing(in, out, listing);

    fclose(in);
    fclose(out);
    if (listing != NULL) { fclose(listing) ; }

    if (res == ASSEMBLY_FAILURE) { log_exit_failure("Error: failed to assemble code given in file '%s'.\n", cfg.src); }

    log_IO("  Assembled to %s", cfg.dst) ;

    return EXIT_SUCCESS;
}
