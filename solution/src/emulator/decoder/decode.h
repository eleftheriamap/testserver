#ifndef __DECODE_H_
#define __DECODE_H_

#include "common/ast.h"
#include "common/word.h"

/**
 * @brief Decodes 32-bit word `c` into an `instr_t`
 * 
 * @param instr_t: The decoded word.
 * @param c: The word to be decoded. 
 * @return true: If the word was decoded successfully.
 */
bool decode_word(instr_t *, word32_t, address_t) ;

#endif
