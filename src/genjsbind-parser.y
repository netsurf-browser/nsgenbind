%{
/* parser for the binding generation config file 
 *
 * This file is part of nsgenjsbind.
 * Licensed under the MIT License,
 *                http://www.opensource.org/licenses/mit-license.php
 * Copyright 2012 Vincent Sanders <vince@netsurf-browser.org>
 */

#include <stdio.h>
#include <string.h>

#include "genjsbind-parser.h"
#include "genjsbind-lexer.h"
#include "webidl-ast.h"
#include "genjsbind-ast.h"

char *errtxt;

 static void genjsbind_error(YYLTYPE *locp, struct genbind_node **genbind_ast, const char *str)
{
    errtxt = strdup(str);
}


%}

%locations
%define api.pure
%error-verbose
%parse-param { struct genbind_node **genbind_ast }

%union
{
  char* text;
  struct genbind_node *node;
}

%token TOK_IDLFILE
%token TOK_HDR_COMMENT
%token TOK_PREAMBLE

%token TOK_BINDING
%token TOK_INTERFACE
%token TOK_TYPE
%token TOK_EXTRA
%token TOK_NODE

%token <text> TOK_IDENTIFIER
%token <text> TOK_STRING_LITERAL
%token <text> TOK_CCODE_LITERAL

%type <text> CBlock

%type <node> Statement
%type <node> Statements
%type <node> IdlFile
%type <node> Preamble 
%type <node> HdrComment
%type <node> Strings
%type <node> Binding
%type <node> BindingArgs
%type <node> BindingArg
%type <node> Type
%type <node> TypeArgs
%type <node> Extra
%type <node> Interface
%type <node> Node

%%

Input
        :
        Statements { *genbind_ast = $1 }
        ;
        

Statements
        : 
        Statement 
        | 
        Statements Statement  
        {
          $$ = genbind_node_link($2, $1);
        }
        | 
        error ';' 
        { 
            fprintf(stderr, "%d: %s\n", yylloc.first_line, errtxt);
            free(errtxt);
            YYABORT ;
        }
        ;

Statement
        : 
        IdlFile
        | 
        HdrComment
        |
        Preamble
        | 
        Binding
        ;

 /* [3] load a web IDL file */
IdlFile
        : 
        TOK_IDLFILE TOK_STRING_LITERAL ';'
        {
          $$ = genbind_new_node(GENBIND_NODE_TYPE_WEBIDLFILE, NULL, $2);
        }
        ;

HdrComment
        : 
        TOK_HDR_COMMENT Strings ';'
        {
          $$ = genbind_new_node(GENBIND_NODE_TYPE_HDRCOMMENT, NULL, $2);
        }
        ;

Strings
        : 
        TOK_STRING_LITERAL
        {
          $$ = genbind_new_node(GENBIND_NODE_TYPE_STRING, NULL, $1);
        }
        | 
        Strings TOK_STRING_LITERAL 
        {
          $$ = genbind_new_node(GENBIND_NODE_TYPE_STRING, $1, $2);
        }
        ;

Preamble
        :
        TOK_PREAMBLE CBlock ';'
        {
          $$ = genbind_new_node(GENBIND_NODE_TYPE_PREAMBLE, NULL, $2);
        }
        ;

CBlock
        : 
        TOK_CCODE_LITERAL
        | 
        CBlock TOK_CCODE_LITERAL 
        {
          $$ = genbind_strapp($1, $2);
        }
        ;

Binding
        :
        TOK_BINDING TOK_IDENTIFIER '{' BindingArgs '}' ';'
        {
          $$ = genbind_new_node(GENBIND_NODE_TYPE_BINDING, 
                                NULL, 
                                genbind_new_node(GENBIND_NODE_TYPE_BINDING_IDENT, $4, $2));
        }
        ;

BindingArgs
        :
        BindingArg
        |
        BindingArgs BindingArg
        {
          $$ = genbind_node_link($2, $1);
        }
        ;

BindingArg
        : 
        Type
        | 
        Extra
        | 
        Interface
        ;

Type
        :
        TOK_TYPE TOK_IDENTIFIER '{' TypeArgs '}' ';'
        {
          $$ = genbind_new_node(GENBIND_NODE_TYPE_TYPE, 
                                NULL, 
                                genbind_new_node(GENBIND_NODE_TYPE_TYPE_IDENT, $4, $2));
        }
        ;

TypeArgs
        :
        /* empty */
        { 
          $$ = NULL;
        }
        |
        Node
        ;

Node
        :
        TOK_NODE TOK_IDENTIFIER ';'
        {
           $$ = genbind_new_node(GENBIND_NODE_TYPE_TYPE_NODE, NULL, $2);
        }
        ;

Extra
        :
        TOK_EXTRA Strings ';'
        {
          $$ = genbind_new_node(GENBIND_NODE_TYPE_TYPE_EXTRA, NULL, $2);
        }
        ;

Interface
        : 
        TOK_INTERFACE TOK_IDENTIFIER ';'
        {
          $$ = genbind_new_node(GENBIND_NODE_TYPE_TYPE_INTERFACE, NULL, $2);
        }
        ;

%%
