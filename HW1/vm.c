// Edrienne Gregana, Andrew Zhao, Chieh-Hui Chang(Jeffrey)
// SRM implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "instruction.h"
#include "machine_types.h"
#include "bof.h"
// #include "bof.c"
#include "asm_unparser.h"
#include "asm.tab.h"
#include "assemble.h"
#include "ast.h"
#include "disasm.h"
#include "disasm.c"
#include "file_location.h"
#include "id_attrs.h"
#include "lexer.h"
#include "parser_types.h"
#include "pass1.h"
#include "regname.h"
#include "symtab.h"
#include "utilities.h"

// a size for the memory (2^16 bytes = 64k)
#define MEMORY_SIZE_IN_BYTES (65536 - BYTES_PER_WORD)
#define MEMORY_SIZE_IN_WORDS (MEMORY_SIZE_IN_BYTES / BYTES_PER_WORD)
static char *progname; // maybe change this? idk what this means


// program counter;
static address_type pc; 
// stack pointer;
// static word_type gp; 
// // Global pointer
// static word_type fp; 
// storing number in each registers
static word_type GPR[32];
static int32_t hi; 
static int32_t lo; 

// will halt with error message if one of these violated:
//PC % BYTES_PER_WORD = 0;
// GPR[$gp] % BYTES_PER_WORD = 0;
// GPR[$sp] % BYTES_PER_WORD = 0;
// GPR[$fp] % BYTES_PER_WORD = 0;
// 0 <= GPR[$gp];
// GPR[$gp] <= GPR[$sp];
// GPR[$sp] <= GPR[$fp];
// GPR[$fp] <= MEMORY_SIZE_IN_BYTES;
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


    if (argc < 2){
        usage();
    }
    // attempt to implement the -p option
    if (strcmp(argv[1], "-p") == 0) {
        const char *bofname = argv[2];
        BOFFILE bf = bof_read_open(bofname);
        // BOFHeader bof_header = bof_read_header(bf);

        // // loading instructions into memory
        // for (PC = 0; PC < bof_header.text_length / BYTES_PER_WORD; PC++) {
        //     bin_instr_t instruction = instruction_read(bf);
        //     memory.instrs[PC] = instruction;
        // }

        // // loading data into memory
        // for (int i = 0; i < bof_header.data_length / BYTES_PER_WORD; i++) {
        //     memory.words[bof_header.data_start_address + i] = bof_read_word(bf);
        // }

        disasmProgram(stdout, bf);
        return EXIT_SUCCESS;
    } else {
        const char *bofname = argv[1];
        BOFFILE bf = bof_read_open(bofname);
        BOFHeader bof_header = bof_read_header(bf);

        pc = bof_header.text_start_address;
        GPR[GP] = bof_header.data_start_address;
        GPR[FP] = bof_header.stack_bottom_addr;
        GPR[SP] = bof_header.stack_bottom_addr;

        // loading instruction into memory
        for (int i = 0; i < bof_header.text_length / BYTES_PER_WORD; i++) {
            bin_instr_t instruction = instruction_read(bf);
            memory.instrs[pc / BYTES_PER_WORD + i] = instruction;
        }

        // loading data into memory
        for (int i = 0; i < bof_header.data_length / BYTES_PER_WORD; i++) {
            memory.words[GPR[GP] / BYTES_PER_WORD + i] = bof_read_word(bf);
        }

    // 3 different types of instructions + system call
    // Register, Immediate, Jump
        
        // use opcode?
        // just need a way to differentiate through the 3 different types.
        //int instr_num = textlength/bytes_per_word
        for (int i = 0; i < bof_header.text_length / BYTES_PER_WORD; i++) {
            bin_instr_t bi = memory.instrs[i];
            instr_type in_type = instruction_type(bi); 
            switch(in_type){
                // system call
                case syscall_instr_type:
                    syscall_type code = instruction_syscall_number(bi);
                    char* sys_call = instruction_syscall_mnemonic(code);
                    
                    if (strcmp(sys_call, 'EXIT') == 0) {
                        exit(0);
                    } else if (strcmp(sys_call, 'PSTR') == 0) {
                        // memory
                        GPR[2] = printf("%s", &memory.bytes[GPR[4]]);
                    } else if (strcmp(sys_call, 'PCH') == 0) {
                        GPR[2] = fputc(GPR[4], stdout);
                    } else if (strcmp(sys_call, 'RCH') == 0) {
                        GPR[2] = getc(stdin);
                    } else if (strcmp(sys_call, 'STRA') == 0) {
                        // start VM tracing; start tracing output

                    } else if (strcmp(sys_call, 'NOTR') == 0) {
                        // no VM tracing; stop the tracing output
                    }
                    break;
                
                // register instructions
                case reg_instr_type:
                    char* reg_call = instruction_func2name(bi);
                
                    int rd = (int)GPR[bi.reg.rd];
                    int rs = (int)GPR[bi.reg.rs];
                    int rt = (int)GPR[bi.reg.rt];

                    if (strcmp(reg_call, 'ADD') == 0) {
                        rd = rs + rt;
                    } else if (strcmp(reg_call, 'SUB') == 0) {
                        rd = rs - rt;
                    } else if (strcmp(reg_call, 'MUL') == 0) {
                        int64_t result = (int64_t)rs * (int64_t)rt;
                        lo = (int32_t)result;
                        hi = (int32_t)(result >> 32);
                    } else if (strcmp(reg_call, 'DIV') == 0) {
                        if (rt == 0) {
                            // error
                        }
                        hi = rs % rt;
                        lo = rs / rt;
                    } else if (strcmp(reg_call, 'MFHI') == 0) {
                        rd = hi;
                    } else if (strcmp(reg_call, 'MFLO') == 0) {
                        rd = lo;
                    } else if (strcmp(reg_call, 'AND') == 0) {
                        rd = rs & rt;
                    } else if (strcmp(reg_call, 'BOR') == 0) {
                        rd = rs | rt;
                    } else if (strcmp(reg_call, 'NOR') == 0) {
                        rd = ~(rs | rt);
                    } else if (strcmp(reg_call, 'XOR') == 0) {
                        rd = rs ^ rt;
                    } else if (strcmp(reg_call, 'SLL') == 0) {
                        rd = rt << bi.reg.shift;
                    } else if (strcmp(reg_call, 'SRL') == 0) {
                        rd = rt >> bi.reg.shift;
                    } else if (strcmp(reg_call, 'JR') == 0) {
                        pc = rs;
                    }    
                    // } else if (strcmp(reg_call, 'SYSCALL') == 0) {
                        
                    // }
                    break;
                // immediate instructions
                case immed_instr_type:
                    char* jump_call = instruction_mnemonic(bi);

                    int rs = (int)GPR[bi.reg.rs];
                    int rt = (int)GPR[bi.reg.rt];
                    immediate_type im = bi.immed.immed;
                    unsigned int zero_ex_im = machine_types_zeroExt(im);
                    int off = machine_types_formOffset(im);

                    if (strcmp(jump_call, 'ADDI') == 0) {
                        rt = rs + machine_types_sgnExt(im);
                    } else if (strcmp(jump_call, 'ANDI') == 0) {
                        rt = rs & zero_ex_im;
                    } else if (strcmp(jump_call, 'BORI') == 0) {
                        rt = rs | zero_ex_im;
                    } else if (strcmp(jump_call, 'XORI') == 0) {
                        rt = rs ^ zero_ex_im;
                    } else if (strcmp(jump_call, 'BEQ') == 0) {
                        if (rs == rt)
                            pc = pc + off;
                    } else if (strcmp(jump_call, 'BGEZ') == 0) {
                        if (rs >= 0)
                            pc = pc + off;
                    } else if (strcmp(jump_call, 'BGTZ') == 0) {
                        if (rs > 0)
                            pc = pc + off;
                    } else if (strcmp(jump_call, 'BLEZ') == 0) {
                        if (rs <= 0)
                            pc = pc + off;
                    } else if (strcmp(jump_call, 'BLTZ') == 0) {
                        if (rs < 0)
                            pc = pc + off;
                    } else if (strcmp(jump_call, 'BNE') == 0) {
                        if (rs != rt)
                            pc = pc + off;
                    } else if (strcmp(jump_call, 'LBU') == 0) {
                        // memory
                        rt = machine_types_zeroExt(memory.bytes[rs + off]);
                    } else if (strcmp(jump_call, 'LW') == 0) {
                        // memory
                        rt = memory.words[(rs + off) / BYTES_PER_WORD];
                    } else if (strcmp(jump_call, 'SB') == 0) {
                        // memory
                        memory.bytes[rs + off] = rt;
                    } else if (strcmp(jump_call, 'SW') == 0) {
                        // memory
                        memory.words[(rs + off) / BYTES_PER_WORD] = rt;
                    }
                    break;
                // jump instructions
                case jump_instr_type:
                    unsigned short op = bi.jump.op;
                    address_type add = bi.jump.addr;
                    if (op == 2) {
                        pc = machine_types_formAddress(pc, add);
                    } else if (op == 3) {
                        GPR[31] = pc;
                        pc = machine_types_formAddress(pc, add);
                    }
                    break;
                default: 
                    perror("instruction type error");
            }
        
        }
    }
}