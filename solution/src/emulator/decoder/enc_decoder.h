#ifndef __ENC_DECODER_H
#define __ENC_DECODER_H

#include <stdlib.h>

#include "utils/log.h"
#include "common/encoded_instrs.h"
#include "common/ast.h"

bool decode_enc_instr(instr_t *, enc_instr i, address_t);

#endif