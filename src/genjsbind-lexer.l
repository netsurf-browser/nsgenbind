%{

/*
 * binding generator lexer
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "genjsbind-parser.h"

#define YY_USER_ACTION yylloc->first_line = yylloc->last_line; \
    yylloc->first_column = yylloc->last_column + 1; \
    yylloc->last_column += yyleng;

%}

/* lexer options */
%option never-interactive
%option yylineno
%option bison-bridge
%option bison-locations
%option nodefault
%option warn
%option prefix="genjsbind_"
%option nounput
%option noinput

/* other Unicode “space separator” */
USP (\xe1\x9a\x80)|(\xe1\xa0\x8e)|(\xe2\x80[\x80-\x8a])|(\xe2\x80\xaf)|(\xe2\x81\x9f)|(\xe3\x80\x80)

/* non breaking space \u00A0 */
NBSP (\xc2\xa0)

/* Line separator \u2028 */
LS (\xe2\x80\xa8)

/* paragraph separator \u2029 */
PS (\xe2\x80\xa9)

whitespace              ([ \t\v\f]|{NBSP}|{USP})

lineend                 ([\n\r]|{LS}|{PS})

multicomment            \/\*(([^*])|(\*[^/]))*\*\/

quotedstring            [^\"\\\n\r]

identifier              [A-Z_a-z][0-9A-Z_a-z]*

other                   [^\t\n\r 0-9A-Z_a-z]

cblockopen              \[\[\[

cblockclose             \]\]\]

%x                      cblock

%%

{whitespace}            ++yylloc->last_column;/* nothing */

{lineend}               if (yytext[0] != '\r') {
                            /* update position counts */
                            ++yylloc->last_line;
                            yylloc->last_column = 0;
                        }

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

\"{quotedstring}*\"     yylval->text = strndup(yytext + 1, yyleng - 2 ); return TOK_STRING_LITERAL;

{multicomment}          /* nothing */

{other}                 return (int) yytext[0];

.                       /* nothing */

<cblock>[^\]]*          yylval->text = strdup(yytext); return TOK_CCODE_LITERAL;
<cblock>{cblockclose}   BEGIN(INITIAL);
<cblock>\]+             yylval->text = strdup(yytext); return TOK_CCODE_LITERAL;

%%
