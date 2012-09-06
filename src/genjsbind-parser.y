/*
 * This is a bison parser for genbind
 *
 */

%{

#include <stdio.h>
#include <string.h>

#include "genjsbind-parser.h"
#include "genjsbind-lexer.h"
#include "webidl-ast.h"
#include "jsapi-binding.h"

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
%token TOK_HDR_COMMENT
%token TOK_INTERFACE

%token <text> TOK_STRING_LITERAL

%type <text> Strings

%%

 /* [1] start with Statements */
Statements
        : Statement 
        | Statements Statement  
        ;

Statement
        : IdlFile
        | HdrComment
        | Interface
        ;

 /* [3] load a web IDL file */
IdlFile
        : TOK_IDLFILE TOK_STRING_LITERAL ';'
        {
          if (webidl_parsefile($2) != 0) {
            YYABORT;
          }
        }
        ;

HdrComment
        : TOK_HDR_COMMENT Strings ';'
        {
          genjsbind_header_comment($2);
        }
        ;

Strings
        : TOK_STRING_LITERAL
        {
          $$ = $1;
        }
        | Strings TOK_STRING_LITERAL 
        {
          char *fullstr;
          int fulllen;
          fulllen = strlen($1) + strlen($2) + 2;
          fullstr = malloc(fulllen);
          snprintf(fullstr, fulllen, "%s\n%s", $1, $2);
          free($1);
          free($2);
          $$ = fullstr;
        }
        ;

Interface
        : TOK_INTERFACE TOK_STRING_LITERAL ';'
        {
          genjsbind_output_interface($2);
        }
        ;

%%
