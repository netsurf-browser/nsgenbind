/*
 * This is a bison parser for genbind
 *
 */

%{

#include <stdio.h>
#include <string.h>

#include "genjsbind-parser.h"
#include "genjsbind-lexer.h"
#include "genjsbind.h"

static void genjsbind_error(const char *str)
{
    fprintf(stderr,"error: %s\n",str);
}


int genjsbind_wrap()
{
    return 1;
}



%}

%define api.pure

%union
{
  char* text;
}

%token TOK_IDLFILE

%token <text>       TOK_STRING_LITERAL

%%

 /* [1] start with Statements */
Statements
        : Statement 
        | Statement Statements 
        ;

Statement
        : IdlFile
        ;

 /* [3] load a web IDL file */
IdlFile
        : TOK_IDLFILE TOK_STRING_LITERAL ';'
        {
          if (loadwebidl($2) != 0) {
            YYABORT;
          }
        }
        ;

%%
