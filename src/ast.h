#ifndef RESULTLANG_AST_H
#define RESULTLANG_AST_H

#include "semantic.h"

#include <stdio.h>

RLAstNode *rl_ast_new(RLAstKind kind, int line);
RLAstNode *rl_ast_number(double value, int line);
RLAstNode *rl_ast_string(const char *value, int line);
RLAstNode *rl_ast_bool(int value, int line);
RLAstNode *rl_ast_identifier(const char *name, int line);
RLAstNode *rl_ast_binary(const char *op, RLAstNode *left, RLAstNode *right, int line);
RLAstNode *rl_ast_assign(const char *name, RLAstNode *value, int line);
RLAstNode *rl_ast_show(RLAstNode *value, int line);
RLAstNode *rl_ast_if(RLAstNode *condition, RLAstNode *then_block, RLAstNode *else_block, int line);
RLAstNode *rl_ast_list_append(RLAstNode *list, RLAstNode *statement);
void rl_ast_print(FILE *out, const RLAstNode *node, int indent);
void rl_ast_free(RLAstNode *node);

#endif
