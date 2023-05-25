#ifndef __LOADER_H
#define __LOADER_H

#include <stdlib.h>
#include <stdbool.h>
#include "wrapper/io.h"

#include "emulator/emulator.h"
#include "common/ast.h"

#define LOAD_SUCCESS 0
#define LOAD_FAIL 1

void free_cpu(cpu_t *cpu) ;

cpu_t *init_cpu(size_t memory_size) ;

uint32_t get_le_word_from_block(memory_block_t *mem, uint64_t i) ;
uint32_t get_le_word(cpu_mem_t *mem, uint64_t i) ;
uint64_t get_le_dword(cpu_mem_t *mem, uint64_t i) ;
uint32_t get_be_word(cpu_mem_t *mem, uint64_t i) ;
uint64_t get_be_dword(cpu_mem_t *mem, uint64_t i) ;

bool store_le_word(cpu_mem_t *mem, uint64_t i, uint32_t w) ;
bool store_le_dword(cpu_mem_t *mem, uint64_t i, uint64_t w) ;

uint64_t get_dword(cpu_t *, address_t) ;
bool set_dword(cpu_t *cpu, address_t, uint64_t w) ;

int load_bin(cpu_mem_t *mem, FILE *in, size_t *count) ;
int store_bin(cpu_mem_t *mem, size_t word_count, FILE *out) ;

#endif