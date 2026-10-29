#ifndef RESULTLANG_SEMANTIC_H
#define RESULTLANG_SEMANTIC_H

#include "symbol_table.h"

#include <stdio.h>

typedef enum {
    RL_AST_PROGRAM,
    RL_AST_LIST,
    RL_AST_ASSIGN,
    RL_AST_SHOW,
    RL_AST_IF,
    RL_AST_NUMBER,
    RL_AST_STRING,
    RL_AST_BOOL,
    RL_AST_IDENTIFIER,
    RL_AST_BINARY
} RLAstKind;

typedef struct RLAstNode {
    RLAstKind kind;
    int line;
    RLType type;
    char text[RL_MAX_NAME];
    double number_value;
    struct RLAstNode *left;
    struct RLAstNode *right;
    struct RLAstNode *third;
    struct RLAstNode *next;
} RLAstNode;

typedef struct {
    RLSymbolTable symbols;
    int error_count;
    FILE *errors;
} RLSemanticContext;

void rl_semantic_init(RLSemanticContext *context, FILE *errors);
int rl_semantic_check_program(RLSemanticContext *context, RLAstNode *program);
RLType rl_semantic_check_expression(RLSemanticContext *context, RLAstNode *expr);

#endif
