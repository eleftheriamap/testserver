#include "emulator/loader.h"
#include "utils/log.h"


cpu_t *__init_cpu(size_t memory_size) ;

/// @brief Free the given memory block
void free_memory_block(memory_block_t *block) {
    if (block != NULL) {
        log_free(block->memory) ;
        log_free(block) ;
    }
}

/// @brief Free the given cpu memory 
void free_mem(cpu_mem_t *mem) {
    if (mem != NULL) {
        free_memory_block(mem->memory) ;
        free_memory_block(mem->IO) ;
        free(mem) ;
    }
}

/// @brief Free the given cpu
void free_cpu(cpu_t *cpu) {
    if (cpu != NULL) {
        free(cpu->pstate);
        free_mem(cpu->memory);
        free(cpu) ;
    }
}

/**
 * @brief Check if the given index `idx` is in the given memory block `block`
 * 
 * @param block The memory block to check
 * @param idx The index to check
 * @return true if the index is in the memory block
 */
static inline
bool in_mem(memory_block_t *block, address_t idx) {
    return block->start <= idx && idx < block->start + block->size ;
}

/**
 * @brief Check if the given index `idx` is a valid memory address, i.e. it is in the main memory or IO memory
 * 
 * @param mem Memory to check
 * @param idx Index to check
 * @return true if the index is a valid memory address
 */
static inline
bool valid_mem_address(cpu_mem_t *mem, address_t idx) {
    return in_mem(mem->memory, idx) || in_mem(mem->IO, idx) ;
}

/**
 * @brief Allocate memory for a new cpu
 * 
 * @param memory_size Size of the main memory to allocate (separate from IO memory)
 * @return cpu_t* CPU pointer with allocated memory
 */
cpu_t *init_cpu(size_t memory_size) {
    cpu_t *res = __init_cpu(memory_size) ;
    if (!res) log_exit_failure("Failed to allocate memory for CPU") ;
    return res ;
}

/// @brief Get the memory at (absolute) index `i`.
#define MEM(mem, i)  (mem->memory[i - mem->start])

/**
 * @brief Get the block of memory that contains the given index `idx`
 * 
 * @param mem The cpu memory
 * @param idx The index to get the block for
 * @return memory_block_t* The memory block that contains the index `idx`
 */
memory_block_t *get_block(cpu_mem_t *mem, address_t idx) {
    if (in_mem(mem->memory, idx)) return mem->memory ;
    if (in_mem(mem->IO, idx)) return mem->IO ;
    log_exit_failure("Out of bounds memory access at 0x%lx", idx) ;
}

/**
 * @brief Store a word `w` in little endian format at the given index `idx`, in the memory block `mem`
 * 
 * @param mem The memory block to store the word in
 * @param idx The absolute index to store the word at (not relative to block start)
 * @param w The word to store
 * @return true if the word was stored successfully
 */
bool store_le_word_in_block(memory_block_t *mem, uint64_t idx, uint32_t w) {
    if (!in_mem(mem, idx)) log_exit_failure("Out of bounds memory write at 0x%lx", idx) ;
    MEM(mem, idx+3) = (uint8_t) ((w & 0xFF000000) >> 24);
    MEM(mem, idx+2) = (uint8_t)((w & 0x00FF0000) >> 16);
    MEM(mem, idx+1) = (uint8_t)((w & 0x0000FF00) >> 8);
    MEM(mem, idx) =   (uint8_t) (w & 0x000000FF);
    return true ;
}

/**
 * @brief Store a word `w` in little endian format at the given index `idx`
 * 
 * @param mem The memory to store the word in
 * @param idx The index to store the word at
 * @param w The word to store
 * @return true if the word was stored successfully
 */
bool store_le_word(cpu_mem_t *mem, uint64_t idx, uint32_t w) {
    if (!valid_mem_address(mem, idx)) log_exit_failure("Out of bounds memory write at 0x%lx", idx) ;

    return store_le_word_in_block(get_block(mem, idx), idx, w) ;
}

/**
 * @brief Store a double word `w` in little endian format at the given index `idx`
 * 
 * @param mem The memory to store the double word in
 * @param idx The index to store the double word at
 * @param w The double word to store
 * @return true if the word was stored successfully
 */
bool store_le_dword(cpu_mem_t *mem, uint64_t idx, uint64_t w) {
    return 
            store_le_word(mem, idx + 4, w >> 32)
        &&  store_le_word(mem, idx, w) ;
}

/**
 * @brief Load a binary file into the the cpu main memory.
 * 
 * @param mem The cpu memory to load the binary into.
 * @param in The file to load the binary from.
 * @param count To report the number of words loaded.
 * @return int load status
 */
int load_bin(cpu_mem_t *mem, FILE *in, size_t *count) {
    size_t read;
    uint32_t w;
    int i = 0;
    while (!feof(in)) {
        read = fread(&w, sizeof(uint32_t), 1, in) ;
        if (read != 1) return LOAD_SUCCESS ;

        store_le_word(mem, i, w);
        i += 4;
        (*count)++;
    }

    return LOAD_SUCCESS ;
}

/**
 * @brief Get the big endian word from the given memory block `mem` at absolute index `i`
 * 
 * @param mem The memory block to get the word from
 * @param i The absolute index to get the word from
 * @return uint32_t The big endian word at the given index
 */
uint32_t get_be_word_from_block(memory_block_t *mem, uint64_t i) {
    uint32_t w = (MEM(mem, i) << 24)
        | (MEM(mem, i+1) << 16)
        | (MEM(mem, i+2) << 8)
        | MEM(mem, i+3) ;
    return w ;
}


// ! TODO: boundary cases
/**
 * @brief Get the big endian word from the cpu memory `mem` at absolute index `i`.
 * 
 * @param mem The cpu memory to get the word from
 * @param i The absolute index to get the word from
 * @return uint32_t The big endian word at the given index
 */
uint32_t get_be_word(cpu_mem_t *mem, uint64_t i) {
    return get_be_word_from_block(get_block(mem, i), i) ;
}

/**
 * @brief Get the big endian double word from the cpu memory `mem` at absolute index `i`.
 * 
 * @param mem The cpu memory to get the double word from
 * @param i The absolute index to get the double word from
 * @return uint64_t The big endian double word at the given index
 */
uint64_t get_be_dword(cpu_mem_t *mem, uint64_t i) {
    return (((uint64_t) get_be_word(mem, i)) << 32) 
        | get_be_word(mem, i + 4) ;
}

/**
 * @brief Get the little endian word from the cpu `cpu` at absolute index `i`.
 * 
 * @param cpu The cpu to get the word from.
 * @param i The absolute index to get the word from.
 * @return uint32_t The little endian word at the given index.
 */
uint32_t get_le_word_mem(cpu_t *cpu, uint64_t i) {
    return get_le_word(cpu->memory, i) ;
}

/**
 * @brief Set the memory of `cpu` at absolute index `i` to the little endian word `w`.
 * 
 * @param cpu The cpu to set the memory of.
 * @param i The absolute index to set the memory at.
 * @param w The little endian word to set the memory to.
 * @return true if the memory was set successfully.
 */
bool set_le_word_mem(cpu_t *cpu, uint64_t i, uint32_t w) {
    return store_le_word(cpu->memory, i, w) ;
}

/**
 * @brief Get the little endian word from the memory block `mem` at absolute index `i`.
 * 
 * @param mem The memory block to get the word from.
 * @param i The absolute index to get the word from.
 * @return uint32_t The little endian word at the given index.
 */
uint32_t get_le_word_from_block(memory_block_t *mem, uint64_t i) {
    if (!in_mem(mem, i)) log_exit_failure("Out of bounds memory read at 0x%lx", i) ;
    uint32_t w = (MEM(mem, i+3) << 24)
        | (MEM(mem, i+2) << 16)
        | (MEM(mem, i+1) << 8)
        | MEM(mem, i) ;
    return w ;
}

/**
 * @brief Get the little endian word from the cpu memory `mem` at absolute index `i`.
 * 
 * @param mem The cpu memory to get the word from.
 * @param i The absolute index to get the word from.
 * @return uint32_t The little endian word at the given index.
 */
uint32_t get_le_word(cpu_mem_t *mem, uint64_t i) {
    return get_le_word_from_block(get_block(mem, i), i) ;
}

/**
 * @brief Get the little endian double word from the cpu memory `mem` at absolute index `i`.
 * 
 * @param mem The cpu memory to get the double word from.
 * @param i The absolute index to get the double word from.
 * @return uint64_t The little endian double word at the given index.
 */
uint64_t get_le_dword(cpu_mem_t *mem, uint64_t i) {
    uint64_t u, l ;
    l = (uint64_t) get_le_word(mem, i) ; 
    u = (uint64_t) get_le_word(mem, i + 4) ;
    return (u << 32) | l ; 
}

/**
 * @brief Get the little endian double word from the memory in cpu `cpu` at absolute index `i`.
 * 
 * @param cpu The cpu to get the double word from.
 * @param i The absolute index to get the double word from.
 * @return uint64_t The little endian double word at the given index.
 */
uint64_t get_dword(cpu_t *cpu, address_t i) {
    return get_le_dword(cpu->memory, i) ;
}

/**
 * @brief Set 8 bytes of memory of `cpu` from absolute index `i` (to `i+8`) to the little endian double word `w`.
 * 
 * @param cpu The cpu to set the memory of.
 * @param i The absolute index to set the memory at.
 * @param w The little endian double word to set the memory to.
 * @return true if the memory was set successfully.
 */
bool set_dword(cpu_t *cpu, address_t i, uint64_t w) {
    return store_le_dword(cpu->memory, i, w) ;
}

/**
 * @brief Write the little endian word `w` to the file `out`.
 * 
 * @param out The file to write to.
 * @param w The little endian word to write.
 * @return int load_status_t LOAD_SUCCESS if the word was written successfully.
 */
int write_w(FILE *out, uint32_t w) {
    uint8_t results[sizeof(uint32_t)];
    size_t written;
    const unsigned  mask = 0xFF; //0xff

    for (int j=0; j < sizeof(w) ;j++){

        results[sizeof(results) - j -1] = w & mask;
        w >>= 8;
    }
    written = fwrite(&results[0], sizeof(results), 1, out);

    if (written != 1) return LOAD_FAIL;
    return LOAD_SUCCESS;
}

/**
 * @brief Write cpu memory `mem` to the file `out` as a binary file.
 * 
 * @param mem The cpu memory to write.
 * @param word_count The number of words to write.
 * @param out The file to write to.
 * @return int Writing status.
 */
int store_bin(cpu_mem_t *mem, size_t word_count, FILE *out) {
    uint32_t w ;
    uint32_t end = word_count * 4 ;
    for (uint32_t i = 0; i < end; i += 4) {
        w = get_le_word(mem, i);
        if (write_w(out, w) == LOAD_FAIL) return LOAD_FAIL;
    }
    return LOAD_SUCCESS;
}

/**
 * @brief Initialize a memory block of size `size` with starting address `start`.
 * 
 * @param size The size of the memory block.
 * @param start The starting address of the memory block.
 * @return memory_block_t* The initialized memory block.
 */
memory_block_t *__init_memory_block(size_t size, address_t start) {
    memory_block_t *block = malloc(sizeof(memory_block_t));
    if (!block) return NULL;

    block->size = size ;
    block->start = start ;
    block->memory = calloc(size, sizeof(memory_t));

    return block ;
}

/**
 * @brief Initialize a cpu memory of size `size`, and its IO page.
 * 
 * @param size The size of the cpu main memory.
 * @return cpu_mem_t* The initialized cpu memory.
 */
cpu_mem_t *__init_cpu_mem(size_t size) {
    cpu_mem_t *mem = malloc(sizeof(cpu_mem_t));
    if (!mem) return NULL;

    mem->IO = __init_memory_block(_4_KB, MAILBOX_PAGE);
    mem->memory = __init_memory_block(size, 0);

    return mem ;
}

/**
 * @brief Initialize a cpu with main memory of size `memory_size`.
 * 
 * @param memory_size The size of the cpu main memory.
 * @return cpu_t* The initialized cpu.
 */
cpu_t *__init_cpu(size_t memory_size) {
    cpu_t *cpu = malloc(sizeof(cpu_t));
    if (cpu == NULL) return NULL;

    // cpu->g_regs = calloc(REG_COUNT, sizeof(reg_t));

    cpu->pstate = calloc(1, sizeof(pstate_t));
    cpu->pstate->Z = true ;

    cpu->pc = 0;
    cpu->sp = 0;
    cpu->fail = false ;
    cpu->halt = false ;
    cpu->get_word_at = *get_le_word_mem ;
    cpu->set_word_at = *set_le_word_mem ;

    cpu->memory = __init_cpu_mem(memory_size);
    if (cpu->memory == NULL) return NULL ;

    if (cpu->g_regs == NULL || cpu->pstate == NULL 
    ||  cpu->memory->memory == NULL || cpu->memory->IO == NULL) {
        free_cpu(cpu);
        return NULL;
    }

    return cpu;

}
