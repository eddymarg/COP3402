// main file for lexer

#include <stdio.h>
#include <stdlib.h>
#include "lexer.h"
#include "parser_types.h"
#include "utilities.h"
#include "ast.h"
#include "pl0.tab.h"


/* Print a usage message on stderr 
   and exit with failure. */
// static void usage(const char *cmdname)
// {
//     fprintf(stderr,
// 	    "Usage: %s code-filename\n",
// 	    cmdname);
//     exit(EXIT_FAILURE);
// }

int main(int argc, char **argv)
{
    if (argc != 2)
        fprintf(stderr, "Incorrect number of arguments\n");
    
    lexer_init(argv[1]);
    lexer_output();

    return 0;
}