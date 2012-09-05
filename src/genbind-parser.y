/*
 * This is a bison parser for genbind
 *
 */

%{

#include <stdio.h>
#include <string.h>

#include "genbind-parser.h"
#include "genbind-lexer.h"

  extern int loadwebidl(char *filename);

void genbind_error(const char *str)
{
        fprintf(stderr,"error: %s\n",str);
}


int genbind_wrap()
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

 /* [1] start with instructions */
Instructions:
        /* empty */
        |
        IdlFile
        ;

IdlFile:
        TOK_IDLFILE TOK_STRING_LITERAL
        {
          loadwebidl($2);
        }
        ;

%%
