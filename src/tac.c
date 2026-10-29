#include "tac.h"

#include <stdio.h>
#include <string.h>

typedef struct {
    char name[RL_MAX_NAME];
    RLType type;
} RLPlace;

static RLPlace place(const char *name, RLType type) {
    RLPlace result;
    snprintf(result.name, sizeof(result.name), "%s", name ? name : "");
    result.type = type;
    return result;
}

void rl_tac_init(RLTacContext *context, FILE *out) {
    if (!context) {
        return;
    }
    context->temp_count = 0;
    context->label_count = 0;
    context->out = out ? out : stdout;
}

static RLPlace generate_expression(RLTacContext *context, const RLAstNode *expr) {
    char buffer[RL_MAX_NAME];

    if (!expr) {
        return place("", RL_TYPE_UNKNOWN);
    }

    switch (expr->kind) {
        case RL_AST_NUMBER:
            snprintf(buffer, sizeof(buffer), "%g", expr->number_value);
            return place(buffer, RL_TYPE_NUMBER);
        case RL_AST_STRING:
            snprintf(buffer, sizeof(buffer), "\"%.110s\"", expr->text);
            return place(buffer, RL_TYPE_STRING);
        case RL_AST_BOOL:
            return place(expr->number_value ? "true" : "false", RL_TYPE_BOOL);
        case RL_AST_IDENTIFIER:
            return place(expr->text, expr->type);
        case RL_AST_BINARY: {
            RLPlace left = generate_expression(context, expr->left);
            RLPlace right = generate_expression(context, expr->right);
            snprintf(buffer, sizeof(buffer), "t%d", ++context->temp_count);
            fprintf(context->out, "%s = %s %s %s\n", buffer, left.name, expr->text, right.name);
            return place(buffer, expr->type);
        }
        default:
            return place("", RL_TYPE_UNKNOWN);
    }
}

void rl_tac_generate_statement_list(RLTacContext *context, const RLAstNode *list) {
    if (!context || !list) {
        return;
    }

    for (const RLAstNode *stmt = list->left; stmt; stmt = stmt->next) {
        if (stmt->kind == RL_AST_ASSIGN) {
            RLPlace value = generate_expression(context, stmt->left);
            fprintf(context->out, "%s = %s\n", stmt->text, value.name);
        } else if (stmt->kind == RL_AST_SHOW) {
            RLPlace value = generate_expression(context, stmt->left);
            fprintf(context->out, "param %s\ncall show\n", value.name);
        } else if (stmt->kind == RL_AST_IF) {
            int else_label = ++context->label_count;
            int end_label = ++context->label_count;
            RLPlace condition = generate_expression(context, stmt->left);
            fprintf(context->out, "ifFalse %s goto L%d\n", condition.name, else_label);
            rl_tac_generate_statement_list(context, stmt->right);
            fprintf(context->out, "goto L%d\n", end_label);
            fprintf(context->out, "L%d:\n", else_label);
            rl_tac_generate_statement_list(context, stmt->third);
            fprintf(context->out, "L%d:\n", end_label);
        }
    }
}

void rl_tac_generate_program(RLTacContext *context, const RLAstNode *program) {
    if (!context || !program) {
        return;
    }

    if (program->kind == RL_AST_PROGRAM) {
        rl_tac_generate_statement_list(context, program->left);
    } else {
        rl_tac_generate_statement_list(context, program);
    }
}
