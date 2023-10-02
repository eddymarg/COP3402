// Edrienne Gregana, Andrew Zhao, Chieh-Hui Chang(Jeffrey)
// SRM implementation

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "instruction.h"
#include "machine_types.h"
#include "bof.h"
#include "asm_unparser.h"
#include "asm.tab.h"
#include "assemble.h"
#include "ast.h"
#include "disasm.h"
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
#define NUM_REG 32


// program counter;
static word_type pc; 
// stack pointer;
static word_type gp; 
// Global pointer
static word_type fp; 
// storing number in each registers
static word_type arr[NUM_REG];

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
        return EXIT_FAILURE;
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

        // loading instruction into memory
        for (int i = 0; i < bof_header.text_length / BYTES_PER_WORD; i++) {
            bin_instr_t instruction = instruction_read(bf);
            memory.instrs[i] = instruction;
        }

        // loading data into memory
        for (int i = 0; i < bof_header.data_length / BYTES_PER_WORD; i++) {
            memory.words[bof_header.data_start_address + i] = bof_read_word(bf);
        }

        pc = bof_header.text_start_address;
        gp = bof_header.data_start_address;
        fp = bof_header.stack_bottom_addr;

    // 3 different types of instructions + system call
    // Register, Immediate, Jump
    while (pc >= 0 && pc < MEMORY_SIZE_IN_WORDS) {
        bin_instr_t curr_instr = memory.instrs[pc];
        instr_type type = instruction_type(curr_instr);

        switch(type) {
            case syscall_instr_type:
                handle_syscall(curr_instr, &pc, gp, memory.words);
                pc++;
                break;
            case reg_instr_type:
                handle_reg_instr(curr_instr);
                pc++;
                break;
            case immed_instr_type:
                handle_immed_instr(curr_instr);
                pc++;
                break;
            case jump_instr_type:
                pc = handle_jmp_instr(curr_instr);
                break;
            default:
                perror("Unknown instruction type");
                return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

void handle_syscall(bin_instr_t instruction) {
    syscall_code code = instruction.immed_data.data.syscall_code;
    switch (code){
        case ADD_SYSCALL:
            break;
        case SUB_SYSCALL:
            break;
        case MUL_SYSCALL:
            break:
        case DIV_SYSCALL:
            break;
        defauly
    }
}

void handle_reg_instr(bin_instr_t instruction) {
    opcode_type opcode = instruction.opcode; //where is opcode_type defined
    reg_idx_type rs = instruction.regs[0];
    reg_idx_type rt = instruction.regs[1];
    reg_idx_type rd = instruction.regs[2];

    switch(opcode) {
        case ADD_OP:
            arr[rd] = arr[rs] + arr[rt];
            break;
        case SUB_OP:
            arr[rd] = arr[rs] - arr[rt];
            break;
        case MUL_OP:
            arr[rd] = arr[rs] * arr[rt];
            break;
        case DIV_OP:
            arr[rd] = arr[rs] / arr[rt];
            break;
        default:
            perror("Unknown register instruction");
            exit(EXIT_FAILURE);   
    } 
}

void handle_immediate_instruction(bin_instr_t instruction) {
    // Handle immediate instructions (e.g., ADDI, SUBI, MULI, DIVI)
    // Extract opcode, registers, and immediate value from instruction
    opcode_type opcode = instruction.opcode;
    reg_idx_type rs = instruction.regs[0];
    reg_idx_type rt = instruction.regs[1];
    int immediate = instruction.immed_data.data.immed;

    // Perform operation based on opcode
    switch (opcode) {
        case ADDI_OP:
            arr[rt] = arr[rs] + immediate;
            break;
        case SUBI_OP:
            arr[rt] = arr[rs] - immediate;
            break;
        case MULI_OP:
            arr[rt] = arr[rs] * immediate;
            break;
        case DIVI_OP:
            arr[rt] = arr[rs] / immediate;
            break;
        default:
            perror("Unknown immediate instruction");
            exit(EXIT_FAILURE);
    }
}

word_type handle_jump_instruction(bin_instr_t instruction) {
    // Handle jump instruction (JMP)
    // Extract jump target address from instruction
    int jump_address = instruction.immed_data.data.lora.addr;

    // Check if the jump address is within valid memory range
    if (jump_address >= 0 && jump_address < MEMORY_SIZE_IN_WORDS) {
        return jump_address;
    } else {
        perror("Invalid jump address");
        exit(EXIT_FAILURE);
    }
}
