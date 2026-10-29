#include "codegen.h"

#include <stdio.h>
#include <string.h>

static void c_escape(FILE *out, const char *text) {
    for (const char *p = text; p && *p; p++) {
        if (*p == '"' || *p == '\\') {
            fputc('\\', out);
        }
        fputc(*p, out);
    }
}

static void emit_expr(FILE *out, const RLAstNode *expr) {
    if (!expr) {
        fputs("0", out);
        return;
    }

    switch (expr->kind) {
        case RL_AST_NUMBER:
            fprintf(out, "%g", expr->number_value);
            break;
        case RL_AST_STRING:
            fputc('"', out);
            c_escape(out, expr->text);
            fputc('"', out);
            break;
        case RL_AST_BOOL:
            fprintf(out, "%d", expr->number_value ? 1 : 0);
            break;
        case RL_AST_IDENTIFIER:
            fputs(expr->text, out);
            break;
        case RL_AST_BINARY:
            if (strcmp(expr->text, "/") == 0) {
                fputs("rl_safe_div(", out);
                emit_expr(out, expr->left);
                fputs(", ", out);
                emit_expr(out, expr->right);
                fputc(')', out);
            } else {
                fputc('(', out);
                emit_expr(out, expr->left);
                fprintf(out, " %s ", expr->text);
                emit_expr(out, expr->right);
                fputc(')', out);
            }
            break;
        default:
            fputs("0", out);
            break;
    }
}

static void emit_indent(FILE *out, int indent) {
    for (int i = 0; i < indent; i++) {
        fputs("    ", out);
    }
}

static void emit_declarations(FILE *out, const RLSymbolTable *symbols) {
    if (!symbols) {
        return;
    }

    for (int i = 0; i < symbols->count; i++) {
        const RLSymbol *symbol = &symbols->entries[i];
        if (symbol->type == RL_TYPE_STRING) {
            fprintf(out, "    const char *%s = \"\";\n", symbol->name);
        } else if (symbol->type == RL_TYPE_BOOL) {
            fprintf(out, "    int %s = 0;\n", symbol->name);
        } else {
            fprintf(out, "    double %s = 0;\n", symbol->name);
        }
    }
}

static void emit_statement_list(FILE *out, const RLAstNode *list, int indent) {
    if (!list) {
        return;
    }

    for (const RLAstNode *stmt = list->left; stmt; stmt = stmt->next) {
        emit_indent(out, indent);
        if (stmt->kind == RL_AST_ASSIGN) {
            fprintf(out, "%s = ", stmt->text);
            emit_expr(out, stmt->left);
            fputs(";\n", out);
        } else if (stmt->kind == RL_AST_SHOW) {
            if (stmt->left && stmt->left->type == RL_TYPE_STRING) {
                fputs("printf(\"%s\\n\", ", out);
                emit_expr(out, stmt->left);
                fputs(");\n", out);
            } else if (stmt->left && stmt->left->type == RL_TYPE_BOOL) {
                fputs("printf(\"%s\\n\", (", out);
                emit_expr(out, stmt->left);
                fputs(") ? \"true\" : \"false\");\n", out);
            } else {
                fputs("printf(\"%.2f\\n\", ", out);
                emit_expr(out, stmt->left);
                fputs(");\n", out);
            }
        } else if (stmt->kind == RL_AST_IF) {
            fputs("if (", out);
            emit_expr(out, stmt->left);
            fputs(") {\n", out);
            emit_statement_list(out, stmt->right, indent + 1);
            emit_indent(out, indent);
            fputs("} else {\n", out);
            emit_statement_list(out, stmt->third, indent + 1);
            emit_indent(out, indent);
            fputs("}\n", out);
        }
    }
}

void rl_codegen_generate_c(FILE *out, const RLAstNode *program, const RLSymbolTable *symbols) {
    if (!out) {
        return;
    }

    fputs("#include <stdio.h>\n", out);
    fputs("#include <stdlib.h>\n\n", out);
    fputs("static double rl_safe_div(double a, double b) {\n", out);
    fputs("    if (b == 0) {\n", out);
    fputs("        fprintf(stderr, \"runtime error: division by zero\\n\");\n", out);
    fputs("        exit(2);\n", out);
    fputs("    }\n", out);
    fputs("    return a / b;\n", out);
    fputs("}\n\n", out);
    fputs("int main(void) {\n", out);
    emit_declarations(out, symbols);
    if (program && program->kind == RL_AST_PROGRAM) {
        emit_statement_list(out, program->left, 1);
    } else {
        emit_statement_list(out, program, 1);
    }
    fputs("    return 0;\n", out);
    fputs("}\n", out);
}
