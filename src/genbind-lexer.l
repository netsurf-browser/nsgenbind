/*
 * binding generator lexer
 */

/* lexer options */
%option outfile="genbind-lexer.c"
%option header-file="genbind-lexer.h"
%option never-interactive
%option bison-bridge
%option nodefault
%option warn
%option debug
%option prefix="genbind_"
%option nounput

/* header block */
%{

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "genbind-parser.h"

%}

 /* other Unicode “space separator” */
USP (\xe1\x9a\x80)|(\xe1\xa0\x8e)|(\xe2\x80[\x80-\x8a])|(\xe2\x80\xaf)|(\xe2\x81\x9f)|(\xe3\x80\x80)

/* non breaking space \u00A0 */
NBSP (\xc2\xa0)

whitespace              ([ \t\v\f\n]|{NBSP}|{USP})

multicomment            \/\*(([^*])|(\*[^/]))*\*\/

quotedstring            [^\"\\\n\r]

other                   [^\t\n\r 0-9A-Z_a-z]

%%

{whitespace}            /* nothing */

webidlfile              return TOK_IDLFILE;

\"{quotedstring}*\"     yylval->text = strndup(yytext + 1,strlen(yytext+1) - 1 ); return TOK_STRING_LITERAL;

{multicomment}          /* nothing */

{other}                 return (int) yytext[0];

.                       /* nothing */

%%