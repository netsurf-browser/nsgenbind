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
%token TOK_PREAMBLE

%token <text> TOK_STRING_LITERAL
%token <text> TOK_CCODE_LITERAL

%%

 /* [1] start with Statements */
Statements
        : Statement 
        | Statements Statement  
        ;

Statement
        : 
        IdlFile
        | 
        HdrComment
        |
        Preamble
        | 
        Interface
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
          
        }
        ;

Strings
        : 
        TOK_STRING_LITERAL
        {
          genjsbind_header_comment($1);
        }
        | 
        Strings TOK_STRING_LITERAL 
        {
          genjsbind_header_comment($2);
        }
        ;

Interface
        : 
        TOK_INTERFACE TOK_STRING_LITERAL ';'
        {
          genjsbind_interface($2);
        }
        ;

Preamble
        :
        TOK_PREAMBLE CBlock
        ;

CBlock
        : 
        TOK_CCODE_LITERAL
        {
          genjsbind_preamble($1);
        }
        | 
        CBlock TOK_CCODE_LITERAL 
        {
          genjsbind_preamble($2);
        }
        ;

%%
