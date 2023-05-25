#include "common/encoded_instrs.h"
#include "emulator/decoder/decode.h"
#include "emulator/decoder/enc_decoder.h"
#include "emulator/decoder/word_decoder.h"

bool decode_word(instr_t *i, word32_t c, address_t pc) {
    enc_instr e ;
    bool dece_status = false ;
    bool decw_status = dec_word(&e, c) ;
    if (decw_status){
        dece_status = decode_enc_instr(i, e, pc) ;
        i->address = pc ;
    }
    
    return decw_status && dece_status ;
}

