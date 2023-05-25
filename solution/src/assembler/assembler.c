#include "wrapper/io.h"

#include "utils/string_funcs.h"
#include "assembler/assembler.h"
#include "assembler/parser/parse.h"
#include "utils/log.h"
#include "assembler/encoder.h"
#include "assembler/worder.h"

int write_to_file (FILE *out, code_word *c) ;
size_t label_pass(assembler_t *asmblr) ;
instr_t line_to_instr(assembler_t *asmblr, char* line) ;
int instr_listing(assembler_t *asmblr, size_t i) ;
void instrs_listing(assembler_t *asmblr) ;
void instrs_to_encs(assembler_t *asmblr) ;
void encs_to_codes(assembler_t *asmblr) ;
int run_assembler(assembler_t *asmblr) ;
void instr_pass(assembler_t *asmblr) ;
int write_instr_listing(assembler_t *asmblr, instr_t instr, code_word c) ;
void assemble_line(assembler_t *asmblr, char *line) ;

/**
 * @brief Logs an error message and returns @c ASSEMBLY_FAILURE .
 * 
 * @param s The error message.
 * @return int The value @c ASSEMBLY_FAILURE .
 */
int assembly_failure(const char *s) {
    log_exit_failure(s) ;
    return ASSEMBLY_FAILURE ;
}


/**
 * @brief Initialises assembler constants.
 * Does not allocate space for the arrays in @c asmblr .
 * @param asmblr The assembler to be initialised.
 */
void init_assembler(assembler_t *asmblr) {
    log_init("Initialising assembler: %p", asmblr) ;
    asmblr->curr_instr = 0;
    asmblr->instrs_size = 0;
}

/**
 * @brief Assembles program @c in, writing to binary file @c out .
 * @pre `in` and `out` are both valid files and already open
*/
int assemble (FILE *in, FILE *out) {
    return assemble_listing(in, out, NULL) ;
}

/**
 * @brief Assembles program @c in, writing to binary file @c out .
 * 
 * Listing is written if @c listing is not @c NULL . 
 * @pre `in` and `out` are both valid files and already open
*/
int assemble_listing(FILE *in, FILE *out, FILE *listing) {
    ASSERT(in != NULL && out != NULL) ;
    assembler_t asmblr = (assembler_t) {.in = in, .out = out, .listing = listing} ;
    init_assembler(&asmblr) ;
    return run_assembler(&asmblr) ;
}


#define FAIL_IF_NULL(x, msg) if (!x) return assembly_failure(msg)

void assemble_all(assembler_t *asmblr) ;

/**
 * @brief This assembler assembles all instructions to encodings, 
 * before converting to 32-bit words, and writing to file.
 * 
 * @param asmblr The assembler configuration.
 * @return int The assembly status.
 */
int run_big_stage_asm(assembler_t *asmblr) {
    loglvl(LOG_2,"Run assembler: %p", asmblr) ;
    int p_init = init_parsing_tables() ;
    if (p_init) return assembly_failure("parser init failed") ;
    
    // Perform first pass for label names
    label_pass(asmblr) ;

    // Allocate space for all instructions, rewind file.
    asmblr->instrs = calloc(asmblr->instrs_size, sizeof(instr_t)) ;
    if (!asmblr->instrs) return assembly_failure("instrs array alloc failed") ;
    rewind(asmblr->in) ;

    // Instructions pass
    instr_pass(asmblr) ;
    free_parsing_tables() ;

    // Instructions to their structured encodings
    asmblr->enc_instrs = calloc(asmblr->instrs_size, sizeof(enc_instr)) ;
    FAIL_IF_NULL(asmblr->enc_instrs, "enc_instrs array alloc failed") ;
    instrs_to_encs(asmblr) ;


    // Structured encodings to 32-bit words
    asmblr->codes = calloc(asmblr->instrs_size, sizeof(word32_t)) ;
    FAIL_IF_NULL(asmblr->codes, "codes array alloc failed") ;
    encs_to_codes(asmblr) ;

    // Dump listing
    instrs_listing(asmblr) ;

    return ASSEMBLY_SUCCESS;
}

/**
 * @brief This assembler assembles each line sequentially.
 * Assembles program @c in, writing to binary file @c out .
 * 
 * @param asmblr The assembler configuration.
 * @return int The assembly status.
 */
int run_line_by_line_asm(assembler_t *asmblr) {
    loglvl(LOG_2,"Run assembler: %p", asmblr) ;
    int p_init = init_parsing_tables() ;
    if (p_init) return assembly_failure("parser init failed") ;
    
    // Perform first pass for label names
    label_pass(asmblr) ;

    // Then assemble each line
    rewind(asmblr->in) ;
    assemble_all(asmblr) ;
    free_parsing_tables() ;

    return ASSEMBLY_SUCCESS;
}

/// @brief Runs assembly pipeline according to configuration in @var asmblr.
/// @param asmblr The assembler configuration
int run_assembler(assembler_t *asmblr) {
    return run_line_by_line_asm(asmblr) ;
}

/// @brief First pass generates the labels in the file, returns number of instructions.
/// @return Number of instructions in file.
size_t label_pass(assembler_t *asmblr) {
    loglvl(LOG_1, "Label pass ...") ;
    size_t curr_addr = 0;
    
    // getline setup
    char *line = NULL ;
    size_t len = 0;
    size_t read ;

    // Check each line for a label
    while ((read = getline(&line, &len, asmblr->in)) != -1) {
        if (is_valid_label(line)) new_label(line, curr_addr * 4) ;
        else if (!not_instr_line(line)) curr_addr++ ;
    }
    if (line) free(line);

    asmblr->instrs_size = curr_addr ;
    //TODO log this
    log_tables() ;
    return curr_addr ;
}

static inline
void inc_addr(assembler_t *asmblr) {
    asmblr->curr_instr++ ;
}

/// @brief add instruction to @c asmblr::instrs, incrementing instr counter.
void push_instr(assembler_t *asmblr, instr_t i) {
    loglvl(LOG_1, "Push instruction: `%s` at index %u", show_instr(i), asmblr->curr_instr) ;
    asmblr->instrs[asmblr->curr_instr] = i ;
    asmblr->curr_instr++ ;
}

/**
 * @brief Assemble, encode and write all instructions to the output file.
 * 
 * @param asmblr The assembler configuration.
 */
void assemble_all(assembler_t *asmblr) {
    loglvl(LOG_1, "Assemble all ...") ;
    asmblr->curr_instr = 0 ;

    // getline setup
    char *line = NULL ;
    size_t len = 0 ;
    size_t read ;

    if (asmblr->listing) {fprintf(asmblr->listing, "0000000000000000 <.data>:\n") ;}

    // Check each line for a label
    while ((read = getline(&line, &len, asmblr->in)) != -1) {
        if (not_instr_line(line)) continue ;
        assemble_line(asmblr, line) ;
        inc_addr(asmblr) ;
    }
    if (line) free(line) ;
}

/**
 * @brief Parses a line into an instruction, encoding it, and writing it to the output file.
 * @param asmblr The assembler configuration.
 * @param line The line to be parsed.
 * @param read The length of the line.
 * 
*/
void assemble_line(assembler_t *asmblr, char *line) {
    loglvl(LOG_0, "Assembling Line: %s", line);

    instr_t i = line_to_instr(asmblr, line) ;

    enc_instr e = encode_instr(i) ;
    word32_t c = to_word_enc(e) ;

    write_to_file(asmblr->out, &c) ;
    if (asmblr->listing) write_instr_listing(asmblr, i, c) ;
}


/// @brief Second pass; parses each instruction and directive 
void instr_pass(assembler_t *asmblr) {
    loglvl(LOG_1, "Instruction pass ...\n") ;
    asmblr->curr_instr = 0 ;
    // getline setup
    char *line = NULL ;
    size_t len = 0;
    size_t read ;

    // Parse each line that is an instruction
    while ((read = getline(&line, &len, asmblr->in)) != -1) {
        if (not_instr_line(line)) continue;
        instr_t i = line_to_instr(asmblr, line) ;
        i.address = asmblr->curr_instr * 4;
        char *c = show_instr(i) ;
        loglvl(LOG_4, "%s\n", c) ;
        log_free(c) ;
        push_instr(asmblr, i) ;
    }
    if (line) free(line);

    ASSERT_M(asmblr->curr_instr <= asmblr->instrs_size, 
        "Pushed too many instructions in `instr_pass`:\n"
            "> current instr index: %u\n"
            "> total instr count  : %u\n", 
        asmblr->curr_instr, asmblr->instrs_size) ;
}

/// @brief Convert each instruction in @c asmblr::instrs to an @c enc_instr, stored @c asmblr::enc_instrs
void instrs_to_encs(assembler_t *asmblr) {
    loglvl(LOG_1, "Encoding pass ...\n") ;
    for (size_t i = 0; i < asmblr->instrs_size; i++) {
        enc_instr e = encode_instr(asmblr->instrs[i]) ;
        (asmblr->enc_instrs)[i] = e ;
    }
}

/// @brief Convert each structured encoding to a 32-bit word. 
void encs_to_codes(assembler_t *asmblr) {
    loglvl(LOG_1, "word32_t pass ... \n") ;

    for (size_t i = 0; i < asmblr->instrs_size; i++)  {
        word32_t c = to_word_enc(asmblr->enc_instrs[i]) ;
        asmblr->codes[i] = c ;
        write_to_file(asmblr->out, &c);
    }
}


/// @brief Write all instructions to the listing file.
void instrs_listing(assembler_t *asmblr) {
    if (!asmblr->listing) return ;
    loglvl(LOG_1, "Listing pass ...") ; 
    
    fprintf(asmblr->listing, "0000000000000000 <.data>:\n") ;

    for (size_t i = 0; i < asmblr->instrs_size; i++)  {
        instr_listing(asmblr, i) ;
    }
    
}

/**
 * @brief Write a single instruction to the listing file.
 * 
 * @param asmblr The assembler configuration.
 * @param idx The index/address of the instruction to write.
 * @param instr The instruction to write.
 * @param c The encoding of the instruction.
 * @return int The write result status.
 */
int write_instr_listing(assembler_t *asmblr, instr_t instr, code_word c) {
    char *instr_s = show_instr(instr) ;
    int res = fprintf(asmblr->listing, "%4lx:\t%08x \t%s\n", (asmblr->curr_instr * 4), c, instr_s) ;
    free(instr_s) ;
    return res ;
}

/**
 * @brief Write a single instruction to the listing file.
 * 
 * @param asmblr The assembler configuration.
 * @param i The index of the instruction to write.
 * @return int Write result status.
 */
int instr_listing(assembler_t *asmblr, size_t i) {
    instr_t instr = asmblr->instrs[i] ;
    code_word c = asmblr->codes[i] ;
    char *instr_s = show_instr(instr) ;
    int res = fprintf(asmblr->listing, "%4lx:\t%08x \t%s\n", (i * 4), c, instr_s) ;
    free(instr_s) ;
    return res ;
}

/**
 * @brief Parse line into an instruction. 
 * 
 * Assumes line has valid instruction syntax; does not account for comments or branch labels.
 * @param line The line to parse.
 * @return instr_t The parsed instruction.
 */
instr_t line_to_instr(assembler_t *asmblr, char* line) {
    instr_t instr = p_instr(line);
    instr.address = asmblr->curr_instr * 4;

    if (log_level_enabled(LOG_3)) {
        char *c = show_instr(instr) ;
        loglvl(LOG_4, "%s\n", c) ;
        log_free(c) ;
    }

    return instr;
}

/**
 * @brief Write single word `c` to file `out`
 * 
 * @param out The file to write to.
 * @param c The pointer to the word to write.
 * @return int Write success status.
 */
int write_to_file(FILE *out, code_word *c) {
    fwrite(c, sizeof(code_word), 1, out);
    return 0;
}
