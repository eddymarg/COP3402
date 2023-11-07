/* $Id: scope_check.c,v 1.16 2023/11/03 16:13:46 leavens Exp $ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scope_check.h"
#include "id_attrs.h"
#include "file_location.h"
#include "ast.h"
#include "utilities.h"
#include "symtab.h"
#include "scope_check.h"

// Build the symbol table for prog
// and check for duplicate declarations
// or uses of undeclared identifiers
void scope_check_program(block_t blk)
{
    symtab_enter_scope();
    scope_check_varDecls(blk.var_decls);
    scope_check_stmt(blk.stmt);
    symtab_leave_scope();
}

// build the symbol table and check the declarations in vds
void scope_check_varDecls(var_decls_t vds)
{
    var_decl_t *vdp = vds.var_decls;
    while (vdp != NULL) {
	scope_check_varDecl(*vdp);
	vdp = vdp->next;
    }
}

// Add declarations for the names in vd,
// reporting duplicate declarations
void scope_check_varDecl(var_decl_t vd)
{
    scope_check_idents(vd.idents, vd.type_tag);
}

// Add declarations for the names in ids
// to current scope as type vt
// reporting any duplicate declarations
void scope_check_idents(idents_t ids,
			AST_type vt)
{
    ident_t *idp = ids.idents;
    while (idp != NULL) {
	scope_check_declare_ident(*idp, vt);
	idp = idp->next;
    }
}

// Add declaration for id
// to current scope as type vt
// reporting if it's a duplicate declaration
void scope_check_declare_ident(ident_t id,
			    AST_type vt)
{
    if (symtab_declared_in_current_scope(id.name)) {
        // only variables in FLOAT
	bail_with_prog_error(*(id.file_loc),
			     "Variable \"%s\" has already been declared!",
			     id.name);
    } else {
	int ofst_cnt = symtab_scope_loc_count();

    if (vt == 4) vt = 0;
    else if (vt == 7) vt = 1;
    else if (vt == 9) vt = 2;
	id_attrs *attrs = create_id_attrs(*(id.file_loc),
					      vt, ofst_cnt);
	symtab_insert(id.name, attrs);
    }
}

// check the statement to make sure that
// all idenfifiers used have been declared
// (if not, then produce an error)
void scope_check_stmt(stmt_t stmt)
{
    switch (stmt.stmt_kind) {
    case assign_stmt:
	scope_check_assignStmt(stmt.data.assign_stmt);
	break;
    case begin_stmt:
	scope_check_beginStmt(stmt.data.begin_stmt);
	break;
    case if_stmt:
	scope_check_ifStmt(stmt.data.if_stmt);
	break;
    case read_stmt:
	scope_check_readStmt(stmt.data.read_stmt);
	break;
    case write_stmt:
	scope_check_writeStmt(stmt.data.write_stmt);
	break;
    case call_stmt:
	scope_check_callStmt(stmt.data.call_stmt);
	break;
    case while_stmt:
	scope_check_whileStmt(stmt.data.while_stmt);
	break;
    case skip_stmt:
	break;
    default:
	bail_with_error("Call to scope_check_stmt with an AST that is not a statement!");
	break;
    }
}

// check the statement for
// undeclared identifiers
void scope_check_assignStmt(
                       assign_stmt_t stmt)
{
    const char *name = stmt.name;
    id_use *idu = scope_check_ident_declared(*(stmt.file_loc),
					     name);
    assert(idu != NULL);  // since would bail if not declared
    scope_check_expr(*(stmt.expr));
}

// check the statement for
// duplicate declarations and for
// undeclared identifiers
void scope_check_beginStmt(begin_stmt_t stmt)
{
    symtab_enter_scope();
    // scope_check_varDecls(stmt.var_decls);
    scope_check_stmts(stmt.stmts);
    symtab_leave_scope();
}

// check the statements to make sure that
// all idenfifiers referenced in them have been declared
// (if not, then produce an error)
void scope_check_stmts(stmts_t stmts)
{
    stmt_t *sp = stmts.stmts;
    while (sp != NULL) {
	scope_check_stmt(*sp);
	sp = sp->next;
    }
}

// check the statement to make sure that
// all idenfifiers referenced in it have been declared
// (if not, then produce an error)
void scope_check_ifStmt(if_stmt_t stmt)
{
    if (stmt.condition.cond_kind == 0)
        scope_check_expr(stmt.condition.data.odd_cond.expr);
    else if (stmt.condition.cond_kind == 1) {
        scope_check_expr(stmt.condition.data.rel_op_cond.expr1);
        scope_check_expr(stmt.condition.data.rel_op_cond.expr2);
    }

    scope_check_stmt(*(stmt.then_stmt));
    scope_check_stmt(*(stmt.else_stmt));
}

void scope_check_whileStmt(while_stmt_t stmt)
{
    if (stmt.condition.cond_kind == 0)
        scope_check_expr(stmt.condition.data.odd_cond.expr);
    else if (stmt.condition.cond_kind == 1) {
        scope_check_expr(stmt.condition.data.rel_op_cond.expr1);
        scope_check_expr(stmt.condition.data.rel_op_cond.expr2);
    }

    scope_check_stmt(*(stmt.body));
}

// check the statement to make sure that
// all idenfifiers referenced in it have been declared
// (if not, then produce an error)
void scope_check_readStmt(read_stmt_t stmt)
{
    scope_check_ident_declared(*(stmt.file_loc), stmt.name);
}

void scope_check_callStmt(call_stmt_t stmt)
{
    scope_check_ident_declared(*(stmt.file_loc), stmt.name);
}

// check the statement to make sure that
// all idenfifiers referenced in it have been declared
// (if not, then produce an error)
void scope_check_writeStmt(write_stmt_t stmt)
{
    scope_check_expr(stmt.expr);
}

// check the expresion to make sure that
// all idenfifiers used have been declared
// (if not, then produce an error)
void scope_check_expr(expr_t exp)
{
    switch (exp.expr_kind) {
    case expr_bin:
	scope_check_binary_op_expr(exp.data.binary);
	break;
    case expr_ident:
	scope_check_ident_expr(exp.data.ident);
	break;
    case expr_number:
	// no identifiers are possible in this case, so just return
	break;
    // case expr_logical_not:
	// scope_check_expr(*(exp.data.logical_not));
	// break;
    default:
	bail_with_error("Unexpected expr_kind_e (%d) in scope_check_expr",
			exp.expr_kind);
	break;
    }
}

// check that all identifiers used in exp
// have been declared
// (if not, then produce an error)
void scope_check_binary_op_expr(binary_op_expr_t exp)
{
    scope_check_expr(*(exp.expr1));
    // (note: no identifiers can occur in the operator)
    scope_check_expr(*(exp.expr2));
}

// check the identifier (id) to make sure that
// all it has been declared (if not, then produce an error)
void scope_check_ident_expr(ident_t id)
{
    scope_check_ident_declared(*(id.file_loc), id.name);
}

// check that name has been declared,
// if so, then return an id_use for it
// otherwise, produce an error 
id_use *scope_check_ident_declared(
         file_location floc,
         const char *name)
{
    id_use *ret = symtab_lookup(name);
    if (ret == NULL) {
	bail_with_prog_error(floc,
				"identifier \"%s\" is not declared!",
				name);
    }
    return ret;
}
