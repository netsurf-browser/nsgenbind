/*
 * binding generator lexer
 */

/* lexer options */
%option never-interactive
%option bison-bridge
%option nodefault
%option warn
%option prefix="genjsbind_"
%option nounput

/* header block */
%{

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "genjsbind-parser.h"

%}

 /* other Unicode “space separator” */
USP (\xe1\x9a\x80)|(\xe1\xa0\x8e)|(\xe2\x80[\x80-\x8a])|(\xe2\x80\xaf)|(\xe2\x81\x9f)|(\xe3\x80\x80)

/* non breaking space \u00A0 */
NBSP (\xc2\xa0)

whitespace              ([ \t\v\f\n]|{NBSP}|{USP})

multicomment            \/\*(([^*])|(\*[^/]))*\*\/

quotedstring            [^\"\\\n\r]

identifier              [A-Z_a-z][0-9A-Z_a-z]*

other                   [^\t\n\r 0-9A-Z_a-z]

cblockopen              \[\[\[

cblockclose             \]\]\]

%x                      cblock

%%

{whitespace}            /* nothing */

 /* terminals */

webidlfile              return TOK_IDLFILE;

hdrcomment              return TOK_HDR_COMMENT;

preamble                return TOK_PREAMBLE;

binding                 return TOK_BINDING;

interface               return TOK_INTERFACE;

type                    return TOK_TYPE;

extra                   return TOK_EXTRA;

node                    return TOK_NODE;

{cblockopen}            BEGIN(cblock);

{identifier}        {
                        /* A leading "_" is used to escape an identifier from 
                         *   looking like a reserved word terminal.
                         */
                        yylval->text = (yytext[0] == '_') ? strdup(yytext + 1) : strdup(yytext);
                        return TOK_IDENTIFIER;
                    }

\"{quotedstring}*\"     yylval->text = strndup(yytext + 1,strlen(yytext+1) - 1 ); return TOK_STRING_LITERAL;

{multicomment}          /* nothing */

{other}                 return (int) yytext[0];

.                       /* nothing */

<cblock>[^\]]*          yylval->text = strdup(yytext); return TOK_CCODE_LITERAL;
<cblock>{cblockclose}   BEGIN(INITIAL);
<cblock>\]+             yylval->text = strdup(yytext); return TOK_CCODE_LITERAL;

%%
