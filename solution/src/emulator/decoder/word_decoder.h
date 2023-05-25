#ifndef __WORD_DECODER_H
#define __WORD_DECODER_H

#include <stdlib.h>

#include "utils/log.h"
#include "common/encoded_instrs.h"
#include "common/word.h"

bool dec_word(enc_instr *, word32_t c);

#endif
