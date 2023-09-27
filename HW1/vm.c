// Edrienne Gregana, Andrew Zhao, (everyone else add names pls)
// SRM implementation

#include <stdio.h>
#include <stdlib.h>
#include "machine_types.h"
#include "instruction.h"
/ / a s i z e f o r t h e memory ( 2 ^ 1 6 b y t e s = 64K )
#define MEMORY_SIZE_IN_BYTES (65536 - BYTES_PER_WORD)
#define MEMORY_SIZE_IN_WORDS (MEMORY_SIZE_IN_BYTES / BYTES_PER_WORD)
static union mem_u {
    byte_type bytes[MEMORY_SIZE_IN_BYTES];
    word_type words[MEMORY_SIZE_IN_WORDS];
    bin_instr_t instrs[MEMORY_SIZE_IN_WORDS];
}memory;

static int PC; // program counter
static int SP; // stack pointer

} memory;

int main(){
    
}
