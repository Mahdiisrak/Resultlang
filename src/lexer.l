%{
#include "parser.tab.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
%}

%option noyywrap yylineno

%%

"show"                  return SHOW;
"if"                    return IF;
"else"                  return ELSE;
"true"                  return TRUE;
"false"                 return FALSE;

[0-9]+(\.[0-9]+)?       { yylval.num = atof(yytext); return NUMBER; }
\"([^\\\"]|\\.)*\"      {
                            yytext[yyleng - 1] = '\0';
                            yylval.str = strdup(yytext + 1);
                            return STRING;
                        }
[A-Za-z_][A-Za-z0-9_]*  { yylval.str = strdup(yytext); return ID; }

">="                    return GE;
"<="                    return LE;
"=="                    return EQ;
"!="                    return NE;
">"                     return '>';
"<"                     return '<';
"+"                     return '+';
"-"                     return '-';
"*"                     return '*';
"/"                     return '/';
"="                     return '=';
"("                     return '(';
")"                     return ')';
"{"                     return '{';
"}"                     return '}';
";"                     return ';';

[ \t\r\n]+              ;
"//".*                  ;
"#".*                   ;

.                       {
                            fprintf(stderr,
                                    "[lexical error] line %d: unknown character '%s'\n",
                                    yylineno, yytext);
                            return BAD_TOKEN;
                        }

%%
