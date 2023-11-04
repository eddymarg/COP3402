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

    program: block periodsym { setProgAST($1); }

    block: constDecls varDecls procDecls stmt {
        $$ = createBlock($1, $2, $3, $4);
    }

    constDecls: constDecl { $$ = $1; } | { $$ = NULL; }

    constDecl: constsym constDefs semisym { $$ = createConstDecl($2); }

    constDefs: constDef { $$ = createConstDefs($1); } | constDefs commasym constDef { $$ = createConstDefs($3); }

    constDef: identsym eqsym numbersym { $$ = createConstDef($1, $3); }

    varDecls: varDecl { $$ = $1; } | { $$ = NULL; }

    varDecl: varsym idents semisym { $$ = createVarDecl($2); }

    idents: identsym { $$ = createIdents($1); } | idents commasym identsym { $$ = createIdents($1); }

    procDecls: procDecl { $$ = $1; } | { $$ = NULL; }

    procDecl: proceduresym identsym semisym block semisym { $$ = createProcDecl($2, $4); }

    stmt: assignStmt { $$ = createStmt($1); } | callStmt { $$ = createStmt($1); } | beginStmt { $$ = createStmt($1); }
    | ifStmt { $$ = createStmt($1); } | whileStmt { $$ = createStmt($1); } | readStmt { $$ = createStmt($1); } | writeStmt { $$ = createStmt($1); }
    | skipStmt { $$ = createStmt($1); }

    assignStmt: identsym becomessym expr { $$ = createAssignStmt($1, $3); }

    callStmt: callsym identsym { $$ = createCallStmt($2); }

    beginStmt: beginsym stmts endsym { $$ = createBeginStmt($2); }

    ifStmt: ifsym condition thensym stmt elsesym stmt { $$ = createIfStmt($2, $4, $6); }

    whileStmt: whilesym condition dosym stmt { $$ = createWhileStmt($2, $4); }

    readStmt: readsym identsym { $$ = createReadStmt($2); }

    writeStmt: writesym expr { $$ = createWriteStmt($2); }

    skipStmt: skipsym { $$ = createSkipStmt(); }

    stmts: stmt { $$ = createStmts($1); } | stmts semisym stmt { $$ = createStmts($3); }

    condition: oddCondition { $$ = createCondition($1); } | relOpCondition { $$ = createCondition($1); }

    oddCondition: oddsym expr { $$ = createOddCondition($2); }

    relOpCondition: expr relOp expr { $$ = createRelOpCondition($1, $2, $3); }

    expr: term { $$ = createExpr($1); } | expr plussym term { $$ = createBinaryExpr("+", $1, $3); }
    | expr minussym term { $$ = createBinaryExpr("-", $1, $3); }

    term: factor { $$ = createTerm($1); } | term multsym factor { $$ = createBinaryTerm("*", $1, $3); }
    | term divsym factor { $$ = createBinaryTerm("/", $1, $3); }

    factor: identsym { $$ = createFactor($1); } | minussym numbersym { $$ = createUnaryFactor("-", createNumberFactor($2)); }
    | posSign numbersym { $$ = createUnaryFactor("+", createNumberFactor($2)); } | lparensym expr rparensym { $$ = $2; }

    relOp: eqsym { $$ = "="; } | neqsym { $$ = "<>"; } | ltsym { $$ = "<"; } | leqsym { $$ = "<="; }
    | gtsym { $$ = ">"; } | geqsym { $$ = ">="; }

    posSign: plussym { $$ = "+"; } | empty { $$ = NULL; }

    empty: /* empty */ { $$ = NULL; }
%%

// Set the program's ast to be ast
void setProgAST(block_t ast) { progast = ast; }
