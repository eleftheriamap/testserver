#include <stdlib.h>

#include "emulator/emulator.h"
#include "emulator/loader.h"
#include "utils/log.h"
#include "utils/file.h"


void setup_emulate_log() {
    set_log_level(LOG_1);
    set_log_output(LOG_STDOUT);
}

int main(int argc, char **argv) {
    setup_emulate_log() ;

    if (argc < 2) log_exit_failure("Usage: %s <binary> [output]\n", argv[0]);

    char *filename = argv[1];
    FILE *in = s_fopen(filename, "rb", "binary file") ;

    cpu_t *cpu = init_cpu(MAXIMUM_MEMORY_SIZE_BYTES) ;

    size_t count = 0 ;
    int load_result = load_bin(cpu->memory, in, &count);
    fclose(in);

    if (load_result == LOAD_FAIL)
        log_exit_failure("Error: failed to load binary data from '%s'.\n", filename);
    
    FILE *out ;
    if (argc >= 3) {
        char *fileout = argv[2] ;
        out = s_fopen(fileout, "wb", "output file");
    } else {
        out = freopen(NULL, "wb", stdout) ;
        if (!stdout) log_exit_failure("Error: failed to reopen standard output in binary mode.\n") ;
    }

    loglvl(LOG_1, "Emulating: %s\n", filename) ;
    emulate(out, cpu, count);
    loglvl(LOG_1, "Emulation Done\n") ;
    f_dump_cpu(out, cpu) ;
    f_dump_mem(out, cpu, 0, count, PRINTM_MEMORY) ;
    free_cpu(cpu);

    return EXIT_SUCCESS;
}
