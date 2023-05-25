#include "common/ast.h"
#include "emulator/emulator.h"
#include "emulator/loader.h"
#include "utils/log.h"
#include "utils/bits.h"
#include "emulator/decoder/decode.h"

// #define TODO(fname) not_implemented_error("Not implemented: %s", fname)
#define TODO_CASE(case) not_implemented_error("Case not implemented: %s")

#define PC_MOVE 4

static inline
void inc_pc(cpu_t *cpu) { 
    cpu->pc += PC_MOVE; 
}



// void get_cpu_reg(cpu_t *cpu, reg_t rn) {
//     if (R0 <= rn && R30 <= rn) return cpu->g_regs[rn] ;
//     switch (rn) {
//         SP: return cpu->sp ;
        
//     }
// }




/// @brief True exactly when @c rn is a general purpose register, 
/// i.e. when @c rn is one of R0-R30. 
static inline
bool is_g_reg(reg_t rn) {
    return R0 <= rn.r && rn.r <= R30 ;
}

static inline
void set_pstate(cpu_t *cpu, bool N, bool Z, bool C, bool V) {
    cpu->pstate->N = N ;
    cpu->pstate->Z = Z ;
    cpu->pstate->C = C ;
    cpu->pstate->V = V ;
}


/**
 * @brief Set the value stored in register `rd` to `val`.
 * If `rd` is the zero register, this write is ignored.
 */
void set_cpu_reg(cpu_t *cpu, reg_t rd, uint64_t val) {
    if (rd.r == RZR) return ;
    g_reg_t *dest;
    if (is_g_reg(rd)) dest = &cpu->g_regs[rd.r] ;
    else switch (rd.r) {
        case SP: dest = &cpu->sp ; break ;
        case PC: dest = &cpu->pc ; break ;
        default: log_error("Bad register: %u", rd.r) ;
    }

    *dest = val ;
    if (!rd.extended) { *dest &= 0xffffffff ; }
}


static
g_reg_t zero_g_reg = 0 ;

/**
 * @brief Get the register `rn`, that is, a pointer to its value.
 * Returns `zero_g_reg` when `rn` is the zero register.
 * ! TODO: Should not be returning null, as that's annoying.
 * ! Maybe should abstract the setting of an individual register?
 */
g_reg_t *get_cpu_reg(cpu_t *cpu, reg_t rn) {
    if (rn.r == RZR) return &zero_g_reg ;
    if (is_g_reg(rn)) return &cpu->g_regs[rn.r] ;
    else switch (rn.r) {
        case SP: return &cpu->sp ;
        case PC: return &cpu->pc ;
        default: FAIL_M("Bad register") ;
    }
}

/**
 * @brief Get the value stored in register `rn`.
 */
static inline
uint64_t get_reg_val(cpu_t *cpu, reg_t rn) {
    uint64_t val = rn.r == RZR ? 0 : *get_cpu_reg(cpu, rn) ;
    if (!rn.extended) val &= 0xffffffff ;
    return val ;
}

#define ulsl(x, n) ((unsigned) x << n)
#define ulsr(x, n) ((unsigned)x >> n)

#define lsl(x, n) (x << n)
#define lsr(x, n) (x >> n)

/// @brief Perform a right rotation on `x` by `shift_amount` bits.
/// @param x - The value to shift.
/// @param shift_amount - The amount to shift by.
/// @return `x` rotated right by `shift_amount` bits.
static inline
uint64_t ror(uint64_t x, uint64_t shift_amount, bool extended) {
    if (extended) { return (x >> shift_amount) | (x << (64 - shift_amount)) ; }
    else {
        x = x & 0xffffffff ; 
        return (x >> shift_amount) | (x << (32 - shift_amount)) ; 
    }
}

// asr(x, n) ((signed)x >> n)

static inline
uint64_t asr(uint64_t x, uint64_t shift_amount, bool extended) {
    if (extended) { 
        return ((int64_t)x) >> ((uint64_t) shift_amount) ; // ! TODO WHY??
    }
    else {
        uint32_t x32 = (uint32_t) (x & 0xffffffff) ; 

        return (uint64_t) ((int32_t) x32 >> ((uint32_t) shift_amount)) ; 
    }
}

/**
 * @brief Shifts `val` according to `sh`, which containing the shift type 
 *  and the amount to shift by.
 * 
 * @param sh: The shift to apply.
 * @param val: The value to shift.
 * @return uint64_t: `val` shifted by `sh`.
 */
uint64_t apply_shift(shift_arg sh, uint64_t val, bool extended) {
    uint64_t res = 0;
    switch (sh.tp) {
    case ASR: 
        res = asr(val, sh.amount, extended) ;
        // ((int64_t)val) >> ((uint64_t) sh.amount) ; 
        // ! TODO: Understand why this is wrong:
        // res = ((signed)val) >> ((uint64_t) sh.amount) ; 
        // setup: val = 8278016, sh.amount = 54
        // Expected: 0, Actual: 1
        // changing signed to int64_t fixes this
        break ;
    case LSL: res = lsl(val, sh.amount) ; break ;
    case LSR: res = lsr(val, sh.amount) ; break ;
    case ROR: res = ror(val, sh.amount, extended) ; break ;
    // default:
        // log_exit_failure("%s - bad shift: %u", __func__, sh.tp) ;
    }
    return res ;
}


/*****************************************************************************/
// Data Processing Emulation

/**
 * @brief Calculates the value of a shifted immediate operand.
 * 
 * @param cpu: The emulated cpu.
 * @param op2: The optionally shifted immediate operand.
 * @return uint64_t: The value calculated from `op2`.
 */
static inline
uint64_t get_imm_sh_val(cpu_t *cpu, op2_imm op2) {
    // op2.imm
    return apply_shift(op2.sh, op2.imm, true) ;
} 

/**
 * @brief Calculates the value of a shifted register operand.
 * 
 * @param cpu: The emulated cpu.
 * @param op2: The optionally shifted register operand.
 * @return uint64_t: The value of op2, i.e. the value of the register, 
 * possibly shifted.
 */
static inline
uint64_t get_reg_sh_val(cpu_t *cpu, op2_reg op2) {
    uint64_t rm_val = get_reg_val(cpu, op2.rm) ;
    uint64_t s = apply_shift(op2.sh, rm_val, op2.rm.extended) ;
    return s ;
}

/**
 * @brief Calculates the numeric value of the operand `op2`.
 */
uint64_t get_op2_val(cpu_t *cpu, op2_t op2) {
    switch (op2.type) {
        case OP2_IMM_SH: return get_imm_sh_val(cpu, op2.imm_sh) ;
        case OP2_REG_SH: return get_reg_sh_val(cpu, op2.reg_sh) ;
        case OP2_MUL: log_exit_failure("Internal error: emulation, get_op2_val: OP2_MUL") ;
    }
    log_exit_failure("get_op2_val: %d", op2) ;
}



/**
 * @brief Emulates a single addition instruction, updating `cpu` to reflect
 * the assignment of the calculation to `rd`, and possibly the 
 * condition flags.
 */
void emulate_add(cpu_t *cpu, instr_dp i) {
#ifndef __SIZEOF_INT128__
    log_error("emulate_add: 128 bit ints not supported") ;
#endif
    uint64_t op2 = get_op2_val(cpu, i.op2) ;
    uint64_t carry = 0 ;
    //  Decide whether to add/subtract and if flags should be set
    bool set_flags = false ;
    switch (i.op_type) {
        case OP_ADD: break ;
        case OP_ADDS: set_flags = true ; break ;
        case OP_SUBS: set_flags = true ; op2 = ~op2 ; carry = 1 ; break ;
        case OP_SUB: op2 = ~op2 ; carry = 1 ; break ;
        default: log_error("emulate_add bad opc: %d", i.op_type) ;
    }

    uint64_t op1 = get_reg_val(cpu, i.rn) ;

    // Calculate the condition flags based on the calculation.
    uint64_t result = op1 + op2 + carry ;
    if (set_flags) {
        __uint128_t usum = (__uint128_t) op1 + (__uint128_t) op2 + ((__uint128_t)carry) ;
        __int128_t sum = (__int128_t) ((signed) op1) + (__int128_t) ((signed) op2) + (__int128_t) ((signed) carry);
        cpu->pstate->N = result >> 63;
        cpu->pstate->Z = result == 0;

        __uint128_t carryCheck = ((__uint128_t) 1 << 64) ;
        cpu->pstate->C = usum >= carryCheck;

        __int128_t overflow = ((__int128_t) 1 << 63) ;
        __int128_t underflow = -((__int128_t) 1 << 63) ;
        cpu->pstate->V = sum >= overflow || sum < underflow;
    }

    set_cpu_reg(cpu, i.rd, result) ;
}


/// @brief Emulate bitwise operation
void emulate_log(cpu_t *cpu, instr_dp i) {
    uint64_t op2 = get_op2_val(cpu, i.op2) ;
    uint64_t op1 = get_reg_val(cpu, i.rn) ;
    uint64_t res = 0 ;
    bool update_pstate = false ;
    switch (i.op_type) {
        case OP_AND: res = op1 & op2 ; break ;
        case OP_BIC: res = op1 & ~op2 ; break ;
        case OP_ORR: res = op1 | op2 ; break ;
        case OP_ORN: res = op1 | ~op2 ; break ;
        case OP_EOR: res = op1 ^ op2 ; break ;
        case OP_EON: res = op1 ^ ~op2 ; break ;
        case OP_ANDS: res = op1 & op2 ; update_pstate = true ; break ;
        case OP_BICS: res = op1 & ~op2 ; update_pstate = true ; break ;
        default: log_error("emulate_log bad opc: %d", i.op_type) ;
    }
    if (update_pstate) set_pstate(cpu, res >> 63, res == 0, 0, 0) ;
    set_cpu_reg(cpu, i.rd, res) ;
}

/// @brief Emulate move operation
void emulate_mov(cpu_t *cpu, instr_dp i) {
    uint64_t imm_val = get_imm_sh_val(cpu, i.op2.imm_sh) ;
    uint64_t res ;
    uint64_t mask = ((uint64_t) 0xffff) << (i.op2.imm_sh.sh.amount);
    switch (i.op_type) {
        case OP_MOVN: res = ~imm_val ; break ;
        case OP_MOVZ: res = imm_val ; break ;
        case OP_MOVK: 
            res = get_reg_val(cpu, i.rd) ; 
            res = (res & ~mask) | (imm_val & mask) ;
            break ;
        default:
            log_error("emulate_mov bad opc: %d", i.op_type) ;
    }
    set_cpu_reg(cpu, i.rd, res) ;
}

/// @brief Emulate multiplication operation
void emulate_mul(cpu_t *cpu, instr_dp i) {
    uint64_t op_n = get_reg_val(cpu, i.rn) ;
    uint64_t op_m = get_reg_val(cpu, i.op2.mul.rm) ;
    uint64_t op_a = get_reg_val(cpu, i.op2.mul.ra) ;

    uint64_t mult = op_n * op_m ;
    uint64_t res ;
    if (i.op_type == OP_MADD) res = op_a + mult ;
    else res = op_a - mult ;

    set_cpu_reg(cpu, i.rd, res) ;
}

/*****************************************************************************/
// Instruction cases

/**
 * @brief Emulates the execution of a single data processing instruction `i`
 * on the CPU `cpu`.
 */
void emulate_dp(cpu_t *cpu, instr_dp i) {
    switch (i.op_type) {
    case ADD_CASES: emulate_add(cpu, i) ; break ;
    case LOG_CASES: emulate_log(cpu, i) ; break ;
    case MOV_CASES: emulate_mov(cpu, i) ; break ;
    case MUL_CASES: emulate_mul(cpu, i) ; break ;
    default:
        log_error("Emulate: Unrecognised opcode: %u", i.op_type) ;
    }
    inc_pc(cpu) ;
}

/**
 * @brief Attempts to emulate the execution of an int directive `x` on
 * the CPU `cpu`. This function always fails as a directive can't be run.
 * 
 * @note The value of `x` could indeed encode a valid instruction; that `x`
 * will be treated as data is an artifact of the AST setup this emulator uses. 
 */
void emulate_int_directive(cpu_t *cpu, uint32_t x) {
    log_error("Attempted to execute int directive: %u", x) ;
}

/// This emulates a `nop` - increments pc.
static inline
void emulate_nop(cpu_t *cpu) {
    inc_pc(cpu) ;
}

/// @brief Evaluates the condition `cond` based on the on the CPU's pstate.
bool check_cond(cpu_t *cpu, cond_e cond) {
    switch (cond) {
    case EQ: return cpu->pstate->Z ;
    case NE: return !(cpu->pstate->Z) ;
    case GE: return cpu->pstate->N == cpu->pstate->V ;
    case LT: return cpu->pstate->N != cpu->pstate->V ;
    case GT: return !(cpu->pstate->Z) && (cpu->pstate->N == cpu->pstate->V) ;
    case LE: return cpu->pstate->Z || (cpu->pstate->N != cpu->pstate->V) ;
    case AL: return true ;
    }
    log_exit_failure("check_cond: Unrecognised condition: %u", cond) ;
}

/// @brief Emulate a branch instruction, setting the PC to the target address (`i.address`).
void emulate_b(cpu_t *cpu, instr_b i) {
    address_t target_address ;
    if (i.tp == TP_BR) target_address = get_reg_val(cpu, i.rn) ;
    else target_address = i.address ;

    bool cond = true ;
    if (i.tp == TP_BCond) cond = check_cond(cpu, i.cond) ;
    if (cond) cpu->pc = target_address ;
    else inc_pc(cpu) ;
}

/// @brief Calculate extended register value. 
int64_t extend_reg(cpu_t *cpu, reg_t r, extend_t e) {
    uint64_t val = get_reg_val(cpu, r) ;
    switch (e.tp) {
    case E_LS_EXTEND_LSL: return lsl(val, e.amount) ;
    case E_LS_EXTEND_SXTX: return  (int64_t) val;
    }
    log_exit_failure("extend_reg: Unrecognised extend type: %u", e.tp) ;
}

/// @brief Calculate the target address of a load/store (immediate offset) instruction.
address_t eval_ls_imm_op(cpu_t *cpu, i_ls_imm i) {
    address_t base_address = get_reg_val(cpu, i.rn) ;
    address_t target_address = base_address ;
    int64_t offset = (signed) i.imm ;

    if (i.idx_tp != IDX_POST) target_address += offset ;

    if (i.idx_tp == IDX_PRE) set_cpu_reg(cpu, i.rn, target_address) ;
    else if (i.idx_tp == IDX_POST) set_cpu_reg(cpu, i.rn, target_address + offset) ;

    return target_address ;
}

/// @brief Calculate the target address of a load/store (register offset) instruction. 
address_t eval_ls_reg_op(cpu_t *cpu, i_ls_reg i) {
    int64_t offset = extend_reg(cpu, i.rm, i.extend) ;
    address_t base_address = get_reg_val(cpu, i.rn) ;
    address_t target_address = base_address + offset ;
    return target_address ;
}

/// @brief Emulate a load/store instruction.
void emulate_ls(cpu_t *cpu, instr_ls i) {
    address_t target ;
    switch (i.arg_tp) {
    case LS_LIT: target = i.lit.lit ; break ;
    case LS_IMM: target = eval_ls_imm_op(cpu, i.imm) ; break ;
    case LS_REG: target = eval_ls_reg_op(cpu, i.reg) ; break ;
    }

    switch (i.op) {
    case OP_STR: set_dword(cpu, target, get_reg_val(cpu, i.rt)) ; break ;
    case OP_LDR: set_cpu_reg(cpu, i.rt, get_dword(cpu, target)) ; break ;
    }

    inc_pc(cpu) ;
}

/*****************************************************************************/
// Emulation of instruction

/**
 * @brief Emulates a single instruction `i` on the chip `cpu`, 
 * updating `cpu`'s state as it would be on an actual chip.
 * 
 * @param cpu: The CPU to be emulated.
 * @param i: The instruction to be run.
 */
void emulate_instr(cpu_t *cpu, instr_t i) {
    switch (i.tp) {
    case I_DP: emulate_dp(cpu, i.dp) ; break ;
    case I_B: emulate_b(cpu, i.b) ; break ;
    case I_LS: emulate_ls(cpu, i.ls) ; break ;
    case I_DIRECTIVE: 
        emulate_int_directive(cpu, i.int_directive) ; break ;
    case I_NOP: emulate_nop(cpu) ; break ;
    default:
        log_exit_failure("emulate_instr: Unrecognised instruction type: %u", i.tp) ;
    }
}


/*****************************************************************************/
// Emulation Ops

/**
 * @brief Returns the word at address `idx` on the cpu's memory.
 * This is just a wrapper for the cpu's `get_word_at` function.
 */
static inline
uint32_t get_word_at(cpu_t *cpu, uint64_t idx) {
    return cpu->get_word_at(cpu, idx) ;
}

/**
 * @brief Sets the word at address `idx` on the cpu's memory to `val`.
 * This is just a wrapper for the cpu's `set_word_at` function.
 * 
 * If the `cpu` doesn't allow writing at the address `idx`, then
 * the function will fail.
 */
static inline
bool set_word_at(cpu_t *cpu, uint64_t idx, uint32_t val) {
    return cpu->set_word_at(cpu, idx, val) ;
}

/**
 * @brief Returns the 32-bit word that `cpu`'s program counter points to.
 */
static inline
word32_t get_next_word(cpu_t *cpu) {
    return get_word_at(cpu, cpu->pc) ;
}

#define HALT_CODE 0x8a000000

static inline
bool is_halt_code(word32_t w) {
    return w == HALT_CODE ;
}

static inline
bool is_halt_instr(instr_t i) {
    return i.tp == I_DP 
    && i.dp.op_type == OP_AND 
    && i.dp.rd.r == R0 && i.dp.rd.extended
    && i.dp.rn.r == R0 && i.dp.rn.extended
    && i.dp.op2.type == OP2_IMM_SH 
    && i.dp.op2.reg_sh.rm.r == R0
    && i.dp.op2.reg_sh.rm.extended ;
}

/**
 * @brief Retrieves and decodes the instruction at the current program counter.
 */
static inline
instr_t fetch_next_instr(cpu_t *cpu) {
    instr_t i ;
    word32_t w = get_word_at(cpu, cpu->pc) ;
    if (is_halt_code(w)) {
        cpu->halt = true ;
        return i ;
    }
    bool decode_status = decode_word(&i, w, cpu->pc) ;
    cpu->fail = !decode_status ;

    if (is_halt_instr(i)) cpu->halt = true ;
    // i.address = cpu->pc ;
    return i ;
}

/**
 * @brief Runs the `cpu` until it either halts or fails. 
 * Execution begins at whatever instruction the PC points to.
 */
void emulate_main(cpu_t *cpu) {
    while (!(cpu->halt || cpu->fail)) {
        instr_t i = fetch_next_instr(cpu) ;
        emulate_instr(cpu, i) ;
    }
}

#define IS_ZERO_INSTR(instr) (instr == 0)

void emulate(FILE *out, cpu_t *cpu, size_t count) {
    // size_t n_instr = count;
    char *s ;
    while (true) {
        if (cpu->fail) {
            log_error("CPU fail\n") ;
            break ;
        }
        instr_t instr = fetch_next_instr(cpu);
        if (cpu->halt) { logln("CPU halt") ; break ; }
        s = show_instr(instr) ;
        loglvl(LOG_1, "(PC: %x) Decoded: %s\n", cpu->pc, s);
        free(s) ;
        emulate_instr(cpu, instr);
        // f_dump_cpu(stdout, cpu) ;
    }
}

/*** Dumps *************************************************************/

void f_dump_cpu(FILE *out, cpu_t *cpu) {

    fprintf(out, "Registers:\n") ;
    for (reg_e rn = R0; rn <= R30; rn++) {
        fprintf(out, "X%02d    = %016lx\n", rn, get_reg_val(cpu, (reg_t) {.r= rn , .extended = true}));
    }
    fprintf(out, "PC     = %016lx\n", cpu->pc);
    
    fprintf(out, "PSTATE : %s%s%s%s\n", 
            cpu->pstate->N ? "N" : "-",
            cpu->pstate->Z ? "Z" : "-",
            cpu->pstate->C ? "C" : "-",
            cpu->pstate->V ? "V" : "-") ;

}

void f_dump_block(FILE *out, memory_block_t *block) {
    uint32_t data ;
    for (address_t i = block->start; i < block->start + block->size; i += 4) {
        data = get_le_word_from_block(block, i);
        if (data) fprintf(out, "0x%08lx : 0x%08x\n", i, data);
    }
}

void f_dump_mem(FILE *out, cpu_t *cpu, uint32_t start, size_t count,
                unsigned char flags) {
    
    fprintf(out, "Non-zero memory:\n");
    f_dump_block(out, cpu->memory->memory);
    f_dump_block(out, cpu->memory->IO);

}
