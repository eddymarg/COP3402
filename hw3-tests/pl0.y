 /* $Id: bison_pl0_y_top.y,v 1.1 2023/10/19 18:47:38 leavens Exp $ */
 /* This file should be named pl0.y, it won't work with other file names! */

%code top {
#include <stdio.h>
}

%code requires {

 /* Including "ast.h" must be at the top, to define the AST type */
#include "ast.h"
#include "machine_types.h"
#include "parser_types.h"
#include "lexer.h"

    /* Report an error to the user on stderr */
extern void yyerror(const char *filename, const char *msg);

}    /* end of %code requires */

%verbose
%define parse.lac full
%define parse.error detailed

 /* the following passes file_name to yyerror,
    and declares it as an formal parameter of yyparse. */
%parse-param { char const *file_name }

%token <ident> identsym
%token <number> numbersym
%token <token> plussym    "+"
%token <token> minussym   "-"
%token <token> multsym    "*"
%token <token> divsym     "/"

%token <token> periodsym  "."
%token <token> semisym    ";"
%token <token> eqsym      "="
%token <token> commasym   ","
%token <token> becomessym ":="

%token <token> constsym   "const"
%token <token> varsym     "var"
%token <token> proceduresym "procedure"
%token <token> callsym    "call"
%token <token> beginsym   "begin"
%token <token> endsym     "end"
%token <token> ifsym      "if"
%token <token> thensym    "then"
%token <token> elsesym    "else"
%token <token> whilesym   "while"
%token <token> dosym      "do"
%token <token> readsym    "read"
%token <token> writesym   "write"
%token <token> skipsym    "skip"
%token <token> oddsym     "odd"

%token <token> neqsym     "<>"
%token <token> ltsym      "<"
%token <token> leqsym     "<="
%token <token> gtsym      ">"
%token <token> geqsym     ">="
%token <token> lparensym  "("
%token <token> rparensym  ")"

%type <block> program
%type <block> block
%type <const_decls> constDecls

%type <var_decls> varDecls
%type <var_decl> varDecl
%type <idents> idents
%type <proc_decls> procDecls
%type <empty> empty
%type <const_decl> constDecl
%type <const_def> constDef
%type <const_defs> constDefs

%type <proc_decl> procDecl
%type <stmt> stmt
%type <assign_stmt> assignStmt
%type <call_stmt> callStmt
%type <begin_stmt> beginStmt
%type <if_stmt> ifStmt
%type <while_stmt> whileStmt
%type <read_stmt> readStmt
%type <write_stmt> writeStmt
%type <skip_stmt> skipStmt
%type <stmts> stmts
%type <condition> condition
%type <odd_condition> oddCondition
%type <rel_op_condition> relOpCondition
%type <expr> expr

%type <token> relOp
%type <expr> term
%type <expr> factor
%type <token> posSign

%start program

%code {
 /* extern declarations provided by the lexer */
extern int yylex(void);

 /* The AST for the program, set by the semantic action 
    for the nonterminal program. */
block_t progast; 

 /* Set the program's ast to be t */
extern void setProgAST(block_t t);
}

%%
 /* Write your grammar rules below and before the next %% */
    
    constDef : "const" identsym "=" numbersym { $$ = ast_const_def($2,$4); } ;

    program : block periodsym { setProgAST($1); } ;

    block : constDecls varDecls procDecls stmt { $$ = ast_block($1, $2, $3, $4); } ;

    constDecls : empty { $$ = ast_const_decls_empty($1); }
            | constDecls constDecl{ $$ = ast_const_decls($1, $2); } ;

    constDecl : constsym constDefs semisym { $$ = ast_const_decl($2); } ;

    constDefs : constDef { $$ = ast_const_defs_singleton($1); }
            | constDefs commasym constDef { $$ = ast_const_defs($1, $3); } ;

    constDef : identsym eqsym numbersym { $$ = ast_const_def($1, $3); } ;

    varDecls : empty { $$ = ast_var_decls_empty($1); }
            | varDecls varDecl { $$ = ast_var_decls($1, $2); } ;

    varDecl : varsym idents semisym { $$ = ast_var_decl($2); } ;

    idents : identsym { $$ = ast_idents_singleton($1); }
            | idents commasym identsym { $$ = ast_idents($1, $3); } ;

    procDecls : empty { $$ = ast_proc_decls_empty($1); }
            | procDecls procDecl { $$ = ast_proc_decls($1, $2); } ;

    procDecl : proceduresym identsym semisym block semisym { $$ = ast_proc_decl($2, $4); } ;

    stmt : assignStmt { $$ = ast_stmt_assign($1); }
        | callStmt { $$ = ast_stmt_call($1); }
        | beginStmt { $$ = ast_stmt_begin($1); }
        | ifStmt { $$ = ast_stmt_if($1); }
        | whileStmt { $$ = ast_stmt_while($1); }
        | readStmt { $$ = ast_stmt_read($1); }
        | writeStmt { $$ = ast_stmt_write($1); }
        | skipStmt { $$ = ast_stmt_skip($1); } ;
        
    assignStmt : identsym becomessym expr { $$ = ast_assign_stmt($1, $3); } ;

    callStmt : callsym identsym { $$ = ast_call_stmt($2); } ;

    beginStmt : beginsym stmts endsym { $$ = ast_begin_stmt($2); } ;

    ifStmt : ifsym condition thensym stmt elsesym stmt { $$ = ast_if_stmt($2, $4, $6); } ;

    whileStmt : whilesym condition dosym stmt { $$ = ast_while_stmt($2, $4); } ; 

    readStmt : readsym identsym { $$ = ast_read_stmt($2); } ;

    writeStmt : writesym expr { $$ = ast_write_stmt($2); } ;

    skipStmt : skipsym { $$ = ast_skip_stmt(file_location_make(lexer_filename(), lexer_line())); } ;

    stmts : stmt { $$ = ast_stmts_singleton($1); } 
        | stmts semisym stmt { $$ = ast_stmts($1, $3); } ;

    condition : oddCondition { $$ = ast_condition_odd($1); }
            | relOpCondition { $$ = ast_condition_rel($1); } ;

    oddCondition : oddsym expr { $$ = ast_odd_condition($2); } ;

    relOpCondition : expr relOp expr { $$ = ast_rel_op_condition($1, $2, $3); } ;

    relOp : "=" | "<>" | "<" | "<=" | ">" | ">=" ;

    expr : term 
        | expr plussym term { $$ = ast_expr_binary_op(ast_binary_op_expr($1, $2, $3)); }
        | expr minussym term { $$ = ast_expr_binary_op(ast_binary_op_expr($1, $2, $3)); } ;

    term : factor 
        | term multsym factor { $$ = ast_expr_binary_op(ast_binary_op_expr($1, $2, $3)); }
        | term divsym factor { $$ = ast_expr_binary_op(ast_binary_op_expr($1, $2, $3)); } ;

    factor : identsym { $$ = ast_expr_ident($1); }
        | minussym numbersym { $$ = ast_expr_negated_number($1, $2); }
        | posSign numbersym { $$ = ast_expr_pos_number($1, $2); }
        | lparensym expr rparensym { $$ = $2; } ;

    posSign : empty {;}
            | plussym ;
    
    empty : %empty { $$ = ast_empty(file_location_make(lexer_filename(), lexer_line())); } ;
    
%%

// Set the program's ast to be ast
void setProgAST(block_t ast) { progast = ast; }
