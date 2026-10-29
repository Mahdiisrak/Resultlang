#ifndef RESULTLANG_CODEGEN_H
#define RESULTLANG_CODEGEN_H

#include "semantic.h"

#include <stdio.h>

void rl_codegen_generate_c(FILE *out, const RLAstNode *program, const RLSymbolTable *symbols);

#endif
