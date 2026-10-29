#ifndef RESULTLANG_TAC_H
#define RESULTLANG_TAC_H

#include "semantic.h"

#include <stdio.h>

typedef struct {
    int temp_count;
    int label_count;
    FILE *out;
} RLTacContext;

void rl_tac_init(RLTacContext *context, FILE *out);
void rl_tac_generate_program(RLTacContext *context, const RLAstNode *program);
void rl_tac_generate_statement_list(RLTacContext *context, const RLAstNode *list);

#endif
