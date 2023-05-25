#ifndef __EMULATOR_H
#define __EMULATOR_H
// #include "arm.h"
// #include "parse.h"
// #include "decoder.h"
// #include "loader.h"
#include "wrapper/io.h"
#include <stdint.h>
#include <stdbool.h>

#include "common/ast.h"

#define REG_COUNT 31 // 0 - 30

typedef enum {
    PRINTM_MEMORY = 0,
    PRINTM_DECODE = 1
} printm_flags_t;

#define MAXIMUM_MEMORY_SIZE_BYTES (2 * 1024 * 1024)

#define MAILBOX_ADDR 0x3f00b880
#define _4_KB (4 * 1024)
#define MAILBOX_PAGE (MAILBOX_ADDR - (MAILBOX_ADDR % _4_KB))

/***********************************************************
* Processor 
*/

// typedef reg reg_t ;
typedef uint64_t g_reg_t;
// typedef uint64_t reg_val ;
typedef uint8_t *memory_t;

typedef struct pstate_t {
    bool N :1 ;
    bool Z :1 ;
    bool C :1 ;
    bool V :1 ;
} pstate_t ;

typedef struct memory_block {
    size_t size ;
    address_t start ;
    memory_t memory ;
}   memory_block_t;

typedef struct cpu_mem_t {
    memory_block_t *memory;
    memory_block_t *IO;
} cpu_mem_t;

typedef struct cpu_t {
    /// @brief The values of general registers R0-R30.
    g_reg_t g_regs[31] ;
    /// @brief The value of the program counter.
    g_reg_t pc;
    /// @brief The value of the stack pointer.
    g_reg_t sp;
    /// @brief The cpu's PSTATE.
    pstate_t *pstate;
    /// @brief True exactly when the cpu failed executing the 
    /// previous instruction.
    bool fail ;
    /// @brief True exactly when the cpu has received a signal to halt
    /// on the previous instruction.
    bool halt ;

    /// @brief The cpu's memory.
    cpu_mem_t *memory ;
    
    ///@brief Accesses the 32-bit word at the address `idx`.
    uint32_t (*get_word_at)(struct cpu_t*, uint64_t idx) ; 
    /// @brief Sets the 32-bit word at address `idx` to `val`.
    bool (*set_word_at)(struct cpu_t*, uint64_t idx, uint32_t val) ;
    /**
     * @brief Auxilary data to be used by different implementations
     * of `get_word_at` and `set_word_at`.
     */
    void *aux ; // For the two methods 
} cpu_t ;


void emulate(FILE *out , cpu_t *cpu , size_t count) ;
void f_dump_mem(FILE *out, cpu_t *cpu, uint32_t start, size_t count,
                unsigned char flags) ;
void f_dump_cpu(FILE *out, cpu_t *cpu) ;

#endif

