#ifndef __BITS_H
#define __BITS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t bit_index ;
typedef uint32_t word32_t ;
typedef bool bit_t ;
typedef uint32_t bits_t ;

#define BIT_MASK(size) ((uint32_t) (((uint64_t) 1 << (size)) - 1))
#define BIT_RANGE(code, hi, lo) ((code >> lo) & (BIT_MASK(hi - lo + 1)))

int64_t sign_extend(uint64_t val, size_t bit_size, size_t new_size) ;

void set_bits_at(word32_t *, bit_index, bits_t) ;
// bit_t bool_to_bit(bool) ;
void set_bit_at(word32_t *, bit_index, bit_t) ;

bits_t bits_at(word32_t c, bit_index high, bit_index low) ;
bit_t bit_at(word32_t c, bit_index idx) ;
bit_t ensure_bit(bit_t b) ;

#define bool_to_bit(b) (!!b)
// ensure_bit(b)

bool bit_at_is(word32_t c, bit_index idx, bit_t b) ;
#endif