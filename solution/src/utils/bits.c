#include "utils/bits.h"

void set_bits_at(word32_t *dest, bit_index low_idx, uint32_t val) {
    *dest |= (val << low_idx) ;
}

void set_bit_at(word32_t *dest, bit_index idx, bool val) {
    *dest ^= (-(bool_to_bit(val)) ^ *dest) & (1 << idx) ; 
    // (bool_to_bit(val) << idx) ;
}

bits_t bits_at(word32_t c, bit_index high, bit_index low) {
    return (c >> low) & BIT_MASK(high - low + 1) ;
}

bit_t bit_at(word32_t c, bit_index idx) {
    return (c >> idx) & 1 ;
}

bit_t ensure_bit(bit_t b) {
    return b ? 1 : 0 ; 
}

bool bit_at_is(word32_t c, bit_index idx, bit_t b) {
    return (bit_at(c, idx) == b) ;
}

int64_t sign_extend(uint64_t val, size_t bit_size, size_t new_size) {
    if (bit_at(val, bit_size)) {
        val |= (0xFFFFFFFFFFFFFFFF << bit_size) ;
    }
    return (int64_t) val ;
}
