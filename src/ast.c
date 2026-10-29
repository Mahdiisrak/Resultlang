#include "ast.h"

#include <stdlib.h>
#include <string.h>

static void copy_text(char *dst, size_t dst_size, const char *src) {
    snprintf(dst, dst_size, "%s", src ? src : "");
}

RLAstNode *rl_ast_new(RLAstKind kind, int line) {
    RLAstNode *node = (RLAstNode *)calloc(1, sizeof(RLAstNode));
    if (!node) {
        return NULL;
    }
    node->kind = kind;
    node->line = line;
    node->type = RL_TYPE_UNKNOWN;
    return node;
}

RLAstNode *rl_ast_number(double value, int line) {
    RLAstNode *node = rl_ast_new(RL_AST_NUMBER, line);
    if (node) {
        node->number_value = value;
        node->type = RL_TYPE_NUMBER;
    }
    return node;
}

RLAstNode *rl_ast_string(const char *value, int line) {
    RLAstNode *node = rl_ast_new(RL_AST_STRING, line);
    if (node) {
        copy_text(node->text, sizeof(node->text), value);
        node->type = RL_TYPE_STRING;
    }
    return node;
}

RLAstNode *rl_ast_bool(int value, int line) {
    RLAstNode *node = rl_ast_new(RL_AST_BOOL, line);
    if (node) {
        node->number_value = value ? 1 : 0;
        node->type = RL_TYPE_BOOL;
        copy_text(node->text, sizeof(node->text), value ? "true" : "false");
    }
    return node;
}

RLAstNode *rl_ast_identifier(const char *name, int line) {
    RLAstNode *node = rl_ast_new(RL_AST_IDENTIFIER, line);
    if (node) {
        copy_text(node->text, sizeof(node->text), name);
    }
    return node;
}

RLAstNode *rl_ast_binary(const char *op, RLAstNode *left, RLAstNode *right, int line) {
    RLAstNode *node = rl_ast_new(RL_AST_BINARY, line);
    if (node) {
        copy_text(node->text, sizeof(node->text), op);
        node->left = left;
        node->right = right;
    }
    return node;
}

RLAstNode *rl_ast_assign(const char *name, RLAstNode *value, int line) {
    RLAstNode *node = rl_ast_new(RL_AST_ASSIGN, line);
    if (node) {
        copy_text(node->text, sizeof(node->text), name);
        node->left = value;
    }
    return node;
}

RLAstNode *rl_ast_show(RLAstNode *value, int line) {
    RLAstNode *node = rl_ast_new(RL_AST_SHOW, line);
    if (node) {
        node->left = value;
    }
    return node;
}

RLAstNode *rl_ast_if(RLAstNode *condition, RLAstNode *then_block, RLAstNode *else_block, int line) {
    RLAstNode *node = rl_ast_new(RL_AST_IF, line);
    if (node) {
        node->left = condition;
        node->right = then_block;
        node->third = else_block;
    }
    return node;
}

RLAstNode *rl_ast_list_append(RLAstNode *list, RLAstNode *statement) {
    if (!list) {
        list = rl_ast_new(RL_AST_LIST, statement ? statement->line : 1);
    }
    if (!statement) {
        return list;
    }
    if (!list->left) {
        list->left = statement;
        return list;
    }
    RLAstNode *tail = list->left;
    while (tail->next) {
        tail = tail->next;
    }
    tail->next = statement;
    return list;
}

static void print_indent(FILE *out, int indent) {
    for (int i = 0; i < indent; i++) {
        fputs("  ", out);
    }
}

void rl_ast_print(FILE *out, const RLAstNode *node, int indent) {
    if (!out || !node) {
        return;
    }

    print_indent(out, indent);
    switch (node->kind) {
        case RL_AST_PROGRAM:
            fputs("Program\n", out);
            rl_ast_print(out, node->left, indent + 1);
            break;
        case RL_AST_LIST:
            fputs("StatementList\n", out);
            for (const RLAstNode *stmt = node->left; stmt; stmt = stmt->next) {
                rl_ast_print(out, stmt, indent + 1);
            }
            break;
        case RL_AST_ASSIGN:
            fprintf(out, "Assign %s\n", node->text);
            rl_ast_print(out, node->left, indent + 1);
            break;
        case RL_AST_SHOW:
            fputs("Show\n", out);
            rl_ast_print(out, node->left, indent + 1);
            break;
        case RL_AST_IF:
            fputs("IfElse\n", out);
            print_indent(out, indent + 1);
            fputs("Condition\n", out);
            rl_ast_print(out, node->left, indent + 2);
            print_indent(out, indent + 1);
            fputs("Then\n", out);
            rl_ast_print(out, node->right, indent + 2);
            print_indent(out, indent + 1);
            fputs("Else\n", out);
            rl_ast_print(out, node->third, indent + 2);
            break;
        case RL_AST_NUMBER:
            fprintf(out, "Number %.2f\n", node->number_value);
            break;
        case RL_AST_STRING:
            fprintf(out, "String \"%s\"\n", node->text);
            break;
        case RL_AST_BOOL:
            fprintf(out, "Bool %s\n", node->number_value ? "true" : "false");
            break;
        case RL_AST_IDENTIFIER:
            fprintf(out, "Identifier %s\n", node->text);
            break;
        case RL_AST_BINARY:
            fprintf(out, "Binary %s : %s\n", node->text, rl_type_name(node->type));
            rl_ast_print(out, node->left, indent + 1);
            rl_ast_print(out, node->right, indent + 1);
            break;
    }
}

void rl_ast_free(RLAstNode *node) {
    if (!node) {
        return;
    }
    rl_ast_free(node->left);
    rl_ast_free(node->right);
    rl_ast_free(node->third);
    rl_ast_free(node->next);
    free(node);
}
