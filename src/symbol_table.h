#ifndef RESULTLANG_SYMBOL_TABLE_H
#define RESULTLANG_SYMBOL_TABLE_H

#include <stdio.h>

#define RL_MAX_SYMBOLS 512
#define RL_MAX_NAME 128

typedef enum {
    RL_TYPE_UNKNOWN = 0,
    RL_TYPE_NUMBER,
    RL_TYPE_STRING,
    RL_TYPE_BOOL
} RLType;

typedef struct {
    char name[RL_MAX_NAME];
    RLType type;
    int first_line;
} RLSymbol;

typedef struct {
    RLSymbol entries[RL_MAX_SYMBOLS];
    int count;
} RLSymbolTable;

void rl_symbol_table_init(RLSymbolTable *table);
const char *rl_type_name(RLType type);
RLSymbol *rl_symbol_find(RLSymbolTable *table, const char *name);
int rl_symbol_define(RLSymbolTable *table, const char *name, RLType type, int line, FILE *errors);
void rl_symbol_table_dump(const RLSymbolTable *table, FILE *out);

#endif
