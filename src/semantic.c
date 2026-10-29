#include "semantic.h"

#include <stdio.h>
#include <string.h>

static FILE *semantic_errors(RLSemanticContext *context) {
    return context && context->errors ? context->errors : stderr;
}

static void semantic_error(RLSemanticContext *context, int line, const char *message) {
    if (context) {
        context->error_count++;
    }
    fprintf(semantic_errors(context), "[semantic error] line %d: %s\n", line, message);
}

static int is_comparison_operator(const char *op) {
    return strcmp(op, ">") == 0 ||
           strcmp(op, "<") == 0 ||
           strcmp(op, ">=") == 0 ||
           strcmp(op, "<=") == 0 ||
           strcmp(op, "==") == 0 ||
           strcmp(op, "!=") == 0;
}

void rl_semantic_init(RLSemanticContext *context, FILE *errors) {
    if (!context) {
        return;
    }
    rl_symbol_table_init(&context->symbols);
    context->error_count = 0;
    context->errors = errors ? errors : stderr;
}

RLType rl_semantic_check_expression(RLSemanticContext *context, RLAstNode *expr) {
    if (!expr) {
        return RL_TYPE_UNKNOWN;
    }

    switch (expr->kind) {
        case RL_AST_NUMBER:
            expr->type = RL_TYPE_NUMBER;
            return expr->type;
        case RL_AST_STRING:
            expr->type = RL_TYPE_STRING;
            return expr->type;
        case RL_AST_BOOL:
            expr->type = RL_TYPE_BOOL;
            return expr->type;
        case RL_AST_IDENTIFIER: {
            RLSymbol *symbol = rl_symbol_find(&context->symbols, expr->text);
            if (!symbol) {
                char message[180];
                snprintf(message, sizeof(message), "undefined variable '%s'", expr->text);
                semantic_error(context, expr->line, message);
                return RL_TYPE_UNKNOWN;
            }
            expr->type = symbol->type;
            return expr->type;
        }
        case RL_AST_BINARY: {
            RLType left = rl_semantic_check_expression(context, expr->left);
            RLType right = rl_semantic_check_expression(context, expr->right);

            if (is_comparison_operator(expr->text)) {
                if (left != RL_TYPE_UNKNOWN && right != RL_TYPE_UNKNOWN && left != right) {
                    char message[180];
                    snprintf(message, sizeof(message),
                             "invalid comparison between %s and %s",
                             rl_type_name(left), rl_type_name(right));
                    semantic_error(context, expr->line, message);
                }
                expr->type = RL_TYPE_BOOL;
                return expr->type;
            }

            if (left != RL_TYPE_NUMBER || right != RL_TYPE_NUMBER) {
                char message[180];
                snprintf(message, sizeof(message),
                         "operator '%s' requires number operands", expr->text);
                semantic_error(context, expr->line, message);
            }

            if (strcmp(expr->text, "/") == 0 &&
                expr->right &&
                expr->right->kind == RL_AST_NUMBER &&
                expr->right->number_value == 0) {
                semantic_error(context, expr->line, "division by zero constant");
            }

            expr->type = RL_TYPE_NUMBER;
            return expr->type;
        }
        default:
            return RL_TYPE_UNKNOWN;
    }
}

static void check_statement_list(RLSemanticContext *context, RLAstNode *list) {
    if (!list) {
        return;
    }

    for (RLAstNode *stmt = list->left; stmt; stmt = stmt->next) {
        if (stmt->kind == RL_AST_ASSIGN) {
            RLType assigned_type = rl_semantic_check_expression(context, stmt->left);
            if (assigned_type != RL_TYPE_UNKNOWN) {
                rl_symbol_define(&context->symbols, stmt->text, assigned_type, stmt->line, semantic_errors(context));
            }
        } else if (stmt->kind == RL_AST_SHOW) {
            rl_semantic_check_expression(context, stmt->left);
        } else if (stmt->kind == RL_AST_IF) {
            RLType condition_type = rl_semantic_check_expression(context, stmt->left);
            if (condition_type != RL_TYPE_BOOL) {
                semantic_error(context, stmt->line, "if condition must be a bool comparison");
            }
            check_statement_list(context, stmt->right);
            check_statement_list(context, stmt->third);
        }
    }
}

int rl_semantic_check_program(RLSemanticContext *context, RLAstNode *program) {
    if (!context || !program) {
        return 0;
    }

    if (program->kind == RL_AST_PROGRAM) {
        check_statement_list(context, program->left);
    } else {
        check_statement_list(context, program);
    }

    return context->error_count == 0;
}
