// Edrienne Gregana, Andrew Zhao, Chieh-Hui Chang(Jeffrey)
// SRM implementation

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "instruction.h"
#include "machine_types.h"
#include "bof.h"
#include "asm_unparser.h";
#include "asm.tab.h";
#include "assemble.h";
#include "ast.h";
#include "disasm.h";
#include "file_location.h";
#include "id_attrs.h";
#include "lexer.h";
#include "parser_types.h";
#include "pass1.h";
#include "regname.h";
#include "symtab.h";
#include "utilities.h":

// a size for the memory (2^16 bytes = 64k)
#define MEMORY_SIZE_IN_BYTES (65536 - BYTES_PER_WORD)
#define MEMORY_SIZE_IN_WORDS (MEMORY_SIZE_IN_BYTES / BYTES_PER_WORD)
static char *progname; // maybe change this? idk what this means

// // this might not be needed
//program counter;
static int PC; 
//stack pointer;
static int SP; 
//Global pointer
static int GP; 

// will halt with error message if one of these violated:
//PC % BYTES_PER_WORD = 0;
// GPR[$gp] % BYTES_PER_WORD = 0;
// GPR[$sp] % BYTES_PER_WORD = 0;
// GPR[$fp] % BYTES_PER_WORD = 0;
// 0 <= GPR[$gp];
// GPR[$gp] <= GPR[$sp];
// GPR[$sp] <= GPR[$fp];
// GPR[$fp] <=MAX_STACK_HEIGHT;
// 0 <= PC
// PC < MEMORY_SIZE_IN_BYTES && GPR[0] = 0

// memory represented in 3 arrays that share the same 
// storage: memory.bytes, memory.words, memory.instrs
// eg. 4 bytes at byte addr 36 can be accessed as 
// memory.bytes[36] or as memory.words[9] or as
// memory.instrs[9]

static union mem_u {
    byte_type bytes[MEMORY_SIZE_IN_BYTES];
    word_type words[MEMORY_SIZE_IN_WORDS];
    bin_instr_t instrs[MEMORY_SIZE_IN_WORDS];
} memory;

// register/computational type instructions
typedef struct {
    unsinged short op : 6;
    reg_num_type rs : 5;
    reg_num_type rd : 5;
    shift_type shift : 5;
    func_type func : 6;
}reg_instr_t;

// system call instructions
typedef struct {
    unsigned short op : 6;
    unsigned int code : 20;
    func_type func : 6;
}syscall_instr_t;

// immediate type instructions
typedef struct {
    unsigned short op : 6;
    reg_num_type rs : 5;
    reg_num_type rt : 5;
    immediate_type immed : 16;
}immed_instr_t;

// jump type instructions
typedef struct {
    unsigned short op : 6;
    address_type addr : 26;
}jump_instr_t;

/*
for (int i = 0; i < j; i++)
{
    memory.instr[i] = instruction_read(bf);
}
*/

void usage() {
    bail_with_error("Usage: %s file.bof", progname);
}

int main(int argc, char *argv[]){
    // order of things:
    // get -p option to work: code in disassembler can be used as model of what to do
    // use functions in BOF module to read from BOF
    /* use bof_read_open to open such file, bof_read_header to read and return header from BOF, instructions_read read each instruction, bof_read_word to read words in data section of BOF
    instruction_assembly_form will be helpful to print instr.
    see code in disassembler for model of how to use
    */
    // once -p option works, implement basic fetch-execute cycle for VM (without -p option)
    // make function that executes single instruction and handles tracing and call function to execute each instruction in a loop


    if (argc != 1){
        usage();
    }

    // attempt to implement the -p option
    if(argv[1] == "-p"){
        progname = argv[0];
        argc--;
        argv++;
        const char *bofname = argv[0];

        BOFFILE bf = bof_read_open(bofname);
        disasmProgram(stdout, bf);
        return EXIT_SUCCESS;
    }
}