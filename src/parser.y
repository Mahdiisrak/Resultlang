%{
#include <stdio.h>
#include <stdlib.h>

int yylex(void);
void yyerror(const char *message);
%}

%union {
    double num;
    char *str;
}

%token <str> ID STRING
%token <num> NUMBER
%token SHOW IF ELSE TRUE FALSE BAD_TOKEN
%token GE LE EQ NE

%left '+' '-'
%left '*' '/'
%nonassoc '>' '<' GE LE EQ NE

%%

program
    : statement_list
    ;

statement_list
    : statement_list statement
    | /* empty */
    ;

statement
    : ID optional_assign expression ';'
    | SHOW '(' expression ')' ';'
    | SHOW expression ';'
    | IF '(' expression ')' block ELSE block
    | IF '(' expression ')' statement ELSE statement
    ;

/* Allows the Presentation-1 shorthand: student "Rahim"; */
optional_assign
    : '='
    | /* empty */
    ;

block
    : '{' statement_list '}'
    | statement
    ;

expression
    : expression '+' expression
    | expression '-' expression
    | expression '*' expression
    | expression '/' expression
    | expression '>' expression
    | expression '<' expression
    | expression GE expression
    | expression LE expression
    | expression EQ expression
    | expression NE expression
    | '(' expression ')'
    | NUMBER
    | STRING
    | TRUE
    | FALSE
    | ID
    ;

%%

void yyerror(const char *message) {
    fprintf(stderr, "[syntax error] %s\n", message);
}
