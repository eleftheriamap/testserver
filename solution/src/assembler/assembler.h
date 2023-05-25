#ifndef __ASSEMBLER_H
#define __ASSEMBLER_H
#include <stdlib.h>
#include "wrapper/io.h"

#include "utils/log.h"
#include "common/ast.h"
#include "common/encoded_instrs.h"
#include "common/word.h"

/*
 * The value returned on a successful assembly operation.
 */
#define ASSEMBLY_SUCCESS  0

/*
 * The value returned should an assembly operation fail.
 */
#define ASSEMBLY_FAILURE  1

typedef struct assembler_t {
    /// Stack pointer for pushing to @c assembler_t::instrs
    size_t curr_instr ;
    /// The number of instructions stored in @c assembler_t::instrs
    size_t instrs_size ;
    /// Array of instructions already read from @c assembler_t::in
    instr_t *instrs ;
    /// Array of structured encodings, encoded from @c assembler_t::instrs
    enc_instr *enc_instrs ;
    /// Array of 32-bit words encoded from @c assembler_t::enc_intrs
    word32_t *codes ;
    /// Assembly @c .as source
    FILE *in ;
    /// Output for instruction binary
    FILE *out ;
    /// Output for code listing. Ignored if null.
    FILE *listing ;
}   assembler_t ;

int assemble(FILE *in, FILE *out);
int assemble_listing(FILE *in, FILE *out, FILE *listing);

#endif