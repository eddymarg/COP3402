#include <limits.h>
#include <string.h>
#include "ast.h"
#include "code.h"
#include "id_use.h"
#include "literal_table.h"
#include "gen_code.h"
#include "utilities.h"
#include "regname.h"
#include "pl0.tab.h"

#define STACK_SPACE 4096

// Initialize the code generator
void gen_code_initialize()
{
    literal_table_initialize();
}

// Requires: bf if open for writing in binary
// and prior to this scope checking and type checking have been done.
// Write all the instructions in cs to bf in order
static void gen_code_output_seq(BOFFILE bf, code_seq cs)
{
    while (!code_seq_is_empty(cs)) {
	bin_instr_t inst = code_seq_first(cs)->instr;
	instruction_write_bin_instr(bf, inst);
	cs = code_seq_rest(cs);
    }
}

// Return a header appropriate for the give code
static BOFHeader gen_code_program_header(code_seq main_cs)
{
    BOFHeader ret;
    strncpy(ret.magic, "FBF", 4);  // for FLOAT SRM
    ret.text_start_address = 0;
    // remember, the unit of length in the BOF format is a byte!
    ret.text_length = code_seq_size(main_cs) * BYTES_PER_WORD;
    // int dsa = MAX(ret.text_length, 1024) + BYTES_PER_WORD;
    int dsa = 4096;
    ret.data_start_address = dsa;
    // ret.ints_length = 0; // FLOAT has no int literals
    ret.data_length = literal_table_size() * BYTES_PER_WORD;
    int sba = 8192;
	// + ret.data_start_address + ret.data_length + STACK_SPACE;
    ret.stack_bottom_addr = sba;
    return ret;
}

static void gen_code_output_literals(BOFFILE bf)
{
    literal_table_start_iteration();
    while (literal_table_iteration_has_next()) {
	word_type w = literal_table_iteration_next();
	// debug_print("Writing literal %f to BOF file\n", w);
	bof_write_word(bf, w);
    }
    literal_table_end_iteration(); // not necessary
}

// Requires: bf is open for writing in binary
// Write the program's BOFFILE to bf
static void gen_code_output_program(BOFFILE bf, code_seq main_cs)
{
    BOFHeader bfh = gen_code_program_header(main_cs);
    bof_write_header(bf, bfh);
    gen_code_output_seq(bf, main_cs);
    gen_code_output_literals(bf);
    bof_close(bf);
}


// Requires: bf if open for writing in binary
// Generate code for prog into bf
void gen_code_program(BOFFILE bf, block_t prog)
{
    code_seq main_cs;
    // We want to make the main program's AR look like all blocks... so:
    // allocate space and initialize any variables
    main_cs = gen_code_var_decls(prog.var_decls);
    main_cs = code_seq_concat(main_cs, gen_code_const_decls(prog.const_decls));
    int vars_len_in_bytes = (code_seq_size(main_cs) / 2) * BYTES_PER_WORD;
    // there is no static link for the program as a whole,
    // so nothing to do for saving FP into A0 as would be done for a block
    main_cs = code_seq_concat(main_cs, code_save_registers_for_AR());
    main_cs = code_seq_concat(main_cs, gen_code_stmt(prog.stmt));
    main_cs = code_seq_concat(main_cs, code_restore_registers_from_AR());
    main_cs = code_seq_concat(main_cs, code_deallocate_stack_space(vars_len_in_bytes));
    main_cs = code_seq_add_to_end(main_cs, code_exit());
    gen_code_output_program(bf, main_cs);
}

code_seq gen_code_block(block_t prog) {

    code_seq main_cs;
    // We want to make the main program's AR look like all blocks... so:
    // allocate space and initialize any variables
    main_cs = gen_code_var_decls(prog.var_decls);
    main_cs = code_seq_concat(main_cs, gen_code_const_decls(prog.const_decls));
    int vars_len_in_bytes
	= (code_seq_size(main_cs) / 2) * BYTES_PER_WORD;
    // there is no static link for the program as a whole,
    // so nothing to do for saving FP into A0 as would be done for a block
    main_cs = code_seq_concat(main_cs, code_save_registers_for_AR());
    main_cs
	= code_seq_concat(main_cs,
			  gen_code_stmt(prog.stmt));
    main_cs = code_seq_concat(main_cs,
			      code_restore_registers_from_AR());
    main_cs = code_seq_concat(main_cs,
			      code_deallocate_stack_space(vars_len_in_bytes));
    main_cs
	= code_seq_add_to_end(main_cs, code_exit());

    return main_cs;
}

code_seq gen_code_const_decls(const_decls_t cds)
{
    code_seq ret = code_seq_empty();

    const_decl_t * cdp = cds.const_decls;
    while(cdp != NULL)
    {
        ret = code_seq_concat(gen_code_const_decl(* cdp), ret);
        cdp = cdp->next;
    }

    return ret;
}

// Generate code for the const-decl, cd
code_seq gen_code_const_decl(const_decl_t cd)
{
    return gen_code_const_defs(cd.const_defs);
}

// Generate code for the const-defs, cdfs
code_seq gen_code_const_defs(const_defs_t cdfs)
{
    code_seq ret = code_seq_empty();

    const_def_t * cdp = cdfs.const_defs;
    while(cdp != NULL)
    {
        ret = code_seq_concat(gen_code_const_def(* cdp), ret);
        cdp = cdp->next;
    }

    return ret;
}

// Generate code for the const-def, cdf
code_seq gen_code_const_def(const_def_t cdf)
{
    code_seq ret = code_allocate_stack_space(BYTES_PER_WORD);

    unsigned int offset = literal_table_lookup(cdf.number.text, cdf.number.value);

    ret = code_seq_concat(ret, code_lw(GP, AT, offset));
    ret = code_seq_concat(ret, code_addi(SP, SP, -4));
    ret = code_seq_concat(ret, code_push_reg_on_stack(AT));

    return ret;
}

// Generate code for the var_decls_t vds to out
// There are 2 instructions generated for each identifier declared
// (one to allocate space and another to initialize that space)
code_seq gen_code_var_decls(var_decls_t vds)
{
    code_seq ret = code_seq_empty();
    var_decl_t *vdp = vds.var_decls;
    while (vdp != NULL) {
	// generate these in reverse order,
	// so the addressing offsets work properly
	ret = code_seq_concat(gen_code_var_decl(*vdp), ret);
	vdp = vdp->next;
    }
    return ret;
}

// Generate code for a single <var-decl>, vd,
// There are 2 instructions generated for each identifier declared
// (one to allocate space and another to initialize that space)
code_seq gen_code_var_decl(var_decl_t vd)
{
    return gen_code_idents(vd.idents);
}

// Generate code for the identififers in idents with type vt
// in reverse order (so the first declared are allocated last).
// There are 2 instructions generated for each identifier declared
// (one to allocate space and another to initialize that space)
code_seq gen_code_idents(idents_t idents)
{
    code_seq ret = code_seq_empty();
    ident_t *idp = idents.idents;
    while (idp != NULL) {
	code_seq alloc_and_init
	    = code_seq_singleton(code_addi(SP, SP,
					   - BYTES_PER_WORD));
    // id_kind vt = idp->idu->attrs->kind;
	// switch (vt) {
	// case constant_idk:
	//     alloc_and_init
	// 	= code_seq_add_to_end(alloc_and_init,
	// 			      code_sw(SP, 0, 0));
	//     break;
	// case variable_idk:
	//     alloc_and_init
	// 	= code_seq_add_to_end(alloc_and_init,
	// 			      code_sw(SP, 0, 0));
	//     break;
	// default:
	//     bail_with_error("Bad type_exp_e (%d) passed to gen_code_idents!",
	// 		    vt);
	//     break;
	// }
	// Generate these in revese order,
	// so addressing works propertly
	ret = code_seq_concat(alloc_and_init, ret);
	idp = idp->next;
    }
    return ret;
}

// Generate code for stmt
code_seq gen_code_stmt(stmt_t stmt)
{
    switch (stmt.stmt_kind) {
    case assign_stmt:
	return gen_code_assign_stmt(stmt.data.assign_stmt);
	break;
    case begin_stmt:
	return gen_code_begin_stmt(stmt.data.begin_stmt);
	break;
    case if_stmt:
	return gen_code_if_stmt(stmt.data.if_stmt);
	break;
    case read_stmt:
	return gen_code_read_stmt(stmt.data.read_stmt);
	break;
    case write_stmt:
	return gen_code_write_stmt(stmt.data.write_stmt);
	break;
    case skip_stmt:
	return gen_code_skip_stmt(stmt.data.skip_stmt);
	break;
    case while_stmt:
	return gen_code_while_stmt(stmt.data.while_stmt);
	break;
    // case call_stmt:
	// return gen_code_call_stmt(stmt.data.call_stmt);
	// break;
    default:
	bail_with_error("Call to gen_code_stmt with an AST that is not a statement!");
	break;
    }
    // The following can never execute, but this quiets gcc's warning
    return code_seq_empty();
}

// Generate code for stmt
code_seq gen_code_assign_stmt(assign_stmt_t stmt)
{
    // can't call gen_code_ident,
    // since stmt.name is not an ident_t
    code_seq ret;
    // put value of expression in $v0
    ret = gen_code_expr(*(stmt.expr));
    assert(stmt.idu != NULL);
    assert(id_use_get_attrs(stmt.idu) != NULL);
    // id_kind typ = id_use_get_attrs(stmt.idu)->kind;
    ret = code_seq_concat(ret, code_pop_stack_into_reg(V0));
    // put frame pointer from the lexical address of the name
    // (using stmt.idu) into $t9
    ret = code_seq_concat(ret,
			  code_compute_fp(T9, stmt.idu->levelsOutward));
    unsigned int offset_count = id_use_get_attrs(stmt.idu)->offset_count;
    assert(offset_count <= USHRT_MAX); // it has to fit!
    // switch (id_use_get_attrs(stmt.idu)->kind) {
    // case constant_idk:
	// ret = code_seq_add_to_end(ret,
	// 			  code_sw(T9, V0, offset_count));
	// break;
    // case variable_idk:
	ret = code_seq_add_to_end(ret, code_sw(T9, V0, offset_count));
	// break;
    // default:
	// bail_with_error("Bad var_type (%d) for ident in assignment stmt!",
	// 		id_use_get_attrs(stmt.idu)->kind);
	// break;
    // }
    return ret;
}

// Generate code for stmt
code_seq gen_code_begin_stmt(begin_stmt_t stmt)
{
    code_seq ret;
    // allocate space and initialize any variables in block
    ret = gen_code_stmts(stmt.stmts);
    // int vars_len_in_bytes = (code_seq_size(ret) / 2) * BYTES_PER_WORD;
    // // in FLOAT, surrounding scope's base is FP, so that is the static link
    // ret = code_seq_add_to_end(ret, code_add(0, FP, A0));
    // // ret = code_seq_concat(ret, code_save_registers_for_AR());
    // ret = code_seq_concat(ret, gen_code_stmts(stmt.stmts));
    // // ret = code_seq_concat(ret, code_restore_registers_from_AR());
    // ret = code_seq_concat(ret, code_deallocate_stack_space(vars_len_in_bytes));
    return ret;
}

// Generate code for the list of statments given by stmts
code_seq gen_code_stmts(stmts_t stmts)
{
    code_seq ret = code_seq_empty();
    stmt_t *sp = stmts.stmts;
    while (sp != NULL) {
	ret = code_seq_concat(ret, gen_code_stmt(*sp));
	sp = sp->next;
    }
    return ret;
}

// Generate code for the if-statment given by stmt
code_seq gen_code_if_stmt(if_stmt_t stmt)
{
    // put truth value of stmt.expr in $v0
    code_seq ret = gen_code_condition(stmt.condition);
    ret = code_seq_concat(ret, code_pop_stack_into_reg(V0));
    code_seq s1 = gen_code_stmt(*(stmt.then_stmt));
    code_seq s2 = gen_code_stmt(*(stmt.else_stmt));
    int s1_len = code_seq_size(s1);
    // skip over body if $v0 contains false
    ret = code_seq_add_to_end(ret, code_beq(V0, 1, s1_len + 1));
    ret = code_seq_concat(ret, s1);

    int s2_len = code_seq_size(s2);
    ret = code_seq_add_to_end(ret, code_beq(0, 0, s2_len));
    ret = code_seq_concat(ret, s2);
    return ret;
}

// Generate code for the read statment given by stmt
code_seq gen_code_read_stmt(read_stmt_t stmt)
{
    // put number read into $v0
    code_seq ret = code_seq_singleton(code_rch());
    // put frame pointer from the lexical address of the name
    // (using stmt.idu) into $t9
    assert(stmt.idu != NULL);
    ret = code_seq_concat(ret,
			  code_compute_fp(T9, stmt.idu->levelsOutward));
    assert(id_use_get_attrs(stmt.idu) != NULL);
    unsigned int offset_count = id_use_get_attrs(stmt.idu)->offset_count;
    assert(offset_count <= USHRT_MAX); // it has to fit!
    ret = code_seq_add_to_end(ret,
			      code_seq_singleton(code_sw(T9, V0, offset_count)));
    return ret;
}

// Generate code for the write statment given by stmt.
code_seq gen_code_write_stmt(write_stmt_t stmt)
{
    // put the result into $a0 to get ready for PCH
    code_seq ret
	= gen_code_expr(stmt.expr);
    ret = code_seq_concat(ret, code_pop_stack_into_reg(A0));
    ret = code_seq_add_to_end(ret, code_pint());
    return ret;
}

code_seq gen_code_skip_stmt(skip_stmt_t stmt)
{
    return NULL;
}

code_seq gen_code_while_stmt(while_stmt_t stmt)
{
    code_seq ret = code_seq_empty();

    // Evaluate the condition and push its truth value on the stack
    code_seq cond_seq = gen_code_condition(stmt.condition);
    int cond_seq_len = code_seq_size(cond_seq);

    // Generate the body of the while loop
    code_seq body_seq = gen_code_stmt(*(stmt.body));
    int body_seq_len = code_seq_size(body_seq);

    // Concatenate condition code at the beginning
    ret = code_seq_concat(ret, cond_seq);

    // Pop the truth value into $v0
    ret = code_seq_add_to_end(ret, code_pop_stack_into_reg(V0));

    // Skip the body of the loop if condition is false
    ret = code_seq_add_to_end(ret, code_beq(V0, 0, body_seq_len + 1));

    // Concatenate the body sequence
    ret = code_seq_concat(ret, body_seq);

    // Jump back to the condition check, which should re-evaluate the condition
    ret = code_seq_add_to_end(ret, code_beq(0, 0, -(cond_seq_len + body_seq_len + 1)));

    return ret;
}

// code_seq gen_code_call_stmt(call_stmt_t stmt)
// {

// }

code_seq gen_code_condition(condition_t cond)
{
    switch (cond.cond_kind) 
    {
        case ck_odd:
            return gen_code_odd_condition(cond.data.odd_cond);
            break;
        case ck_rel:
            return gen_code_rel_op_condition(cond.data.rel_op_cond);
            break;
        default:
            bail_with_error("Unexpected cond_kind_e (%d) in gen_code_condition", cond.cond_kind);
            break;
    }

    return code_seq_empty();
}

code_seq gen_code_odd_condition(odd_condition_t cond)
{
    code_seq ret = gen_code_expr(cond.expr);
    return ret;
}

code_seq gen_code_rel_op_condition(rel_op_condition_t cond)
{
    code_seq ret = gen_code_expr(cond.expr1);
    ret = code_seq_concat(ret, gen_code_rel_op(cond.rel_op));
    ret = code_seq_concat(ret, gen_code_expr(cond.expr2));
    return ret;
}

// Generate code for the expression exp
// putting the result on top of the stack,
// and using V0 and AT as temporary registers
// May also modify SP, HI,LO when executed
code_seq gen_code_expr(expr_t exp)
{
    switch (exp.expr_kind) {
    case expr_bin:
	return gen_code_binary_op_expr(exp.data.binary);
	break;
    case expr_ident:
	return gen_code_ident(exp.data.ident);
	break;
    case expr_number:
	return gen_code_number(exp.data.number);
	break;
    // case expr_logical_not:
	// return gen_code_logical_not_expr(*(exp.data.logical_not));
	// break;
    default:
	bail_with_error("Unexpected expr_kind_e (%d) in gen_code_expr",
			exp.expr_kind);
	break;
    }
    // never happens, but suppresses a warning from gcc
    return code_seq_empty();
}

// Generate code for the expression exp
// putting the result on top of the stack,
// and using V0 and AT as temporary registers
// May also modify SP, HI,LO when executed
code_seq gen_code_binary_op_expr(binary_op_expr_t exp)
{
    // put the values of the two subexpressions on the stack
    code_seq ret = gen_code_expr(*(exp.expr1));
    ret = code_seq_concat(ret, gen_code_expr(*(exp.expr2)));
    // check the types match
    // type_exp_e t1 = ast_expr_type(*(exp.expr1));
    // assert(ast_expr_type(*(exp.expr2)) == t1);
    // do the operation, putting the result on the stack
    ret = code_seq_concat(ret, gen_code_arith_op(exp.arith_op));
    return ret;
}

// Generate code to apply op to the
// 2nd from top and top of the stack,
// putting the result on top of the stack in their place,
// and using V0 and AT as temporary registers
// Modifies SP when executed
code_seq gen_code_op(token_t op)
{
    switch (op.code) {
    case eqsym: case neqsym:
    case ltsym: case leqsym:
    case gtsym: case geqsym:
	return gen_code_rel_op(op);
	break;
    case plussym: case minussym: case multsym: case divsym:
	// assert(typ == float_te);
	return gen_code_arith_op(op);
	break;
    default:
	bail_with_error("Unknown token code (%d) in gen_code_op",
			op.code);
	break;
    }
    return code_seq_empty();
}

// Generate code to apply the floating-point arith_op to the
// 2nd from top and top of the stack,
// putting the result on top of the stack in their place,
// and using V0 and AT as temporary registers
// Also modifies SP when executed
code_seq gen_code_arith_op(token_t arith_op)
{
    // load top of the stack (the second operand) into AT
    code_seq ret = code_pop_stack_into_reg(AT);
    // load next element of the stack into V0
    ret = code_seq_concat(ret, code_pop_stack_into_reg(V0));

    code_seq do_op = code_seq_empty();
    switch (arith_op.code) {
    case plussym:
	do_op = code_seq_add_to_end(do_op, code_add(V0, AT, V0));
	break;
    case minussym:
	do_op = code_seq_add_to_end(do_op, code_sub(V0, AT, V0));
	break;
    case multsym:
	do_op = code_seq_add_to_end(do_op, code_mul(V0, AT));
	break;
    case divsym:
	do_op = code_seq_add_to_end(do_op, code_div(V0, AT));
	break;
    default:
	bail_with_error("Unexpected arithOp (%d) in gen_code_arith_op",
			arith_op.code);
	break;
    }
    do_op = code_seq_concat(do_op, code_push_reg_on_stack(V0));
    return code_seq_concat(ret, do_op);
}

// Generate code for the rel_op
// applied to 2nd from top and top of the stack,
// putting the result on top of the stack in their place,
// and using V0 and AT as temporary registers
// Also modifies SP when executed
code_seq gen_code_rel_op(token_t rel_op)
{
    // load top of the stack (the second operand) into AT
    code_seq ret = code_pop_stack_into_reg(AT);
    // load next element of the stack into V0
    ret = code_seq_concat(ret, code_pop_stack_into_reg(V0));

    // start out by doing the comparison
    // and skipping the next 2 instructions if it's true
    code_seq do_op = code_seq_empty();
    switch (rel_op.code) {
    case eqsym: 
	    do_op = code_seq_singleton(code_beq(V0, AT, 2));
	break;
    case neqsym:
	    do_op = code_seq_singleton(code_bne(V0, AT, 2));
	break;
    case ltsym:
	    do_op = code_seq_singleton(code_sub(V0, AT, V0));
	    do_op = code_seq_add_to_end(do_op, code_bltz(V0, 2));
	break;
    case leqsym:
	    do_op = code_seq_singleton(code_sub(V0, AT, V0));
	    do_op = code_seq_add_to_end(do_op, code_blez(V0, 2));
	break;
    case gtsym:
	    do_op = code_seq_singleton(code_sub(V0, AT, V0));
	    do_op = code_seq_add_to_end(do_op, code_bgtz(V0, 2));
	break;
    case geqsym:
	    do_op = code_seq_singleton(code_sub(V0, AT, V0));
	    do_op = code_seq_add_to_end(do_op, code_bgez(V0, 2));
	break;
    default:
	bail_with_error("Unknown token code (%d) in gen_code_rel_op",
			rel_op.code);
	break;
    }
    ret = code_seq_concat(ret, do_op);
    // rest of the code for the comparisons
    ret = code_seq_add_to_end(ret, code_add(0, 0, AT)); // put false in AT
    ret = code_seq_add_to_end(ret, code_beq(0, 0, 1)); // skip next instr
    ret = code_seq_add_to_end(ret, code_addi(0, AT, 1)); // put true in AT
    ret = code_seq_concat(ret, code_push_reg_on_stack(AT));
    return ret;
}

// Generate code to put the value of the given identifier
// on top of the stack
// Modifies T9, V0, and SP when executed
code_seq gen_code_ident(ident_t id)
{
    assert(id.idu != NULL);
    code_seq ret = code_compute_fp(T9, id.idu->levelsOutward);
    assert(id_use_get_attrs(id.idu) != NULL);
    unsigned int offset_count = id_use_get_attrs(id.idu)->offset_count;
    assert(offset_count <= USHRT_MAX); // it has to fit!
    // id_kind typ = id_use_get_attrs(id.idu)->kind;
    // if (typ == variable_idk) {
	ret = code_seq_add_to_end(ret, code_lw(T9, V0, offset_count));
    // } else {
	// ret = code_seq_add_to_end(ret,
	// 			  code_lw(T9, V0, offset_count));
    // }
    return code_seq_concat(ret, code_push_reg_on_stack(V0));
}

// Generate code to put the given number on top of the stack
// Modifies V0 when executed
code_seq gen_code_number(number_t num)
{
    unsigned int global_offset
	= literal_table_lookup(num.text, num.value);
    return code_seq_concat(code_seq_singleton(code_lw(GP, V0, global_offset)),
			   code_push_reg_on_stack(V0));
}

// Generate code for the expression exp
// putting the result on top of the stack,
// and using V0 and AT as temporary registers
// May also modify SP, HI,LO when executed
code_seq gen_code_logical_not_expr(expr_t exp)
{
    code_seq ret = gen_code_expr(exp);
    ret = code_seq_concat(ret, code_pop_stack_into_reg(AT));
    // if 0 skip next 2 instructions
    ret = code_seq_add_to_end(ret, code_beq(0, AT, 2));
    // it was 1, so put 0 in AT
    ret = code_seq_add_to_end(ret, code_add(0, 0, AT));
    // and skip the next instruction
    ret = code_seq_add_to_end(ret, code_beq(0, 0, 1));
    // put 1 in AT
    ret = code_seq_add_to_end(ret, code_addi(0, AT, 1));
    // push the result on the stack
    ret = code_seq_concat(ret, code_push_reg_on_stack(AT));
    return ret;
}
