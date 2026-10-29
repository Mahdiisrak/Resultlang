#include "symbol_table.h"

#include <stdio.h>
#include <string.h>

void rl_symbol_table_init(RLSymbolTable *table) {
    if (!table) {
        return;
    }
    table->count = 0;
}

const char *rl_type_name(RLType type) {
    switch (type) {
        case RL_TYPE_NUMBER:
            return "number";
        case RL_TYPE_STRING:
            return "string";
        case RL_TYPE_BOOL:
            return "bool";
        default:
            return "unknown";
    }
}

RLSymbol *rl_symbol_find(RLSymbolTable *table, const char *name) {
    if (!table || !name) {
        return NULL;
    }

    for (int i = 0; i < table->count; i++) {
        if (strcmp(table->entries[i].name, name) == 0) {
            return &table->entries[i];
        }
    }
    return NULL;
}

int rl_symbol_define(RLSymbolTable *table, const char *name, RLType type, int line, FILE *errors) {
    if (!table || !name) {
        return 0;
    }

    RLSymbol *existing = rl_symbol_find(table, name);
    if (existing) {
        if (existing->type != type) {
            fprintf(errors ? errors : stderr,
                    "[semantic error] line %d: type mismatch for '%s': was %s, assigned %s\n",
                    line, name, rl_type_name(existing->type), rl_type_name(type));
            return 0;
        }
        return 1;
    }

    if (table->count >= RL_MAX_SYMBOLS) {
        fprintf(errors ? errors : stderr,
                "[semantic error] line %d: symbol table capacity exceeded\n", line);
        return 0;
    }

    RLSymbol *slot = &table->entries[table->count++];
    snprintf(slot->name, sizeof(slot->name), "%s", name);
    slot->type = type;
    slot->first_line = line;
    return 1;
}

void rl_symbol_table_dump(const RLSymbolTable *table, FILE *out) {
    if (!table || !out) {
        return;
    }

    fprintf(out, "Name,Type,FirstLine\n");
    for (int i = 0; i < table->count; i++) {
        fprintf(out, "%s,%s,%d\n",
                table->entries[i].name,
                rl_type_name(table->entries[i].type),
                table->entries[i].first_line);
    }
}
