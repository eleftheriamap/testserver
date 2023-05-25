#ifndef __PARSE_H
#define __PARSE_H
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include "wrapper/io.h"
#include <string.h>

#include "utils/log.h"
#include "common/ast.h"


instr_t p_instr(char *line) ;
int init_parsing_tables() ;

void new_label(char *line, address_t address) ;
extern bool not_instr_line(char *) ;
extern bool is_valid_label(char *) ;
extern bool is_comment(char *) ;
void log_tables() ;

void free_parsing_tables() ;

#endif