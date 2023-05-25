#ifndef __ENCODER_H
#define __ENCODER_H

#include <stdlib.h>
#include "wrapper/io.h"

#include "common/ast.h"
#include "common/encoded_instrs.h"

enc_instr encode_instr(instr_t i) ;

#endif