#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKENS 8192
#define MAX_TEXT 256
#define MAX_NODES 4096
#define MAX_SYMBOLS 512
#define MAX_LINES 8192

typedef enum {
    TOK_EOF, TOK_ID, TOK_NUM, TOK_STR, TOK_TRUE, TOK_FALSE, TOK_SHOW, TOK_IF, TOK_ELSE,
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_ASSIGN, TOK_GT, TOK_LT, TOK_GE,
    TOK_LE, TOK_EQ, TOK_NE, TOK_LP, TOK_RP, TOK_LB, TOK_RB, TOK_SEMI, TOK_BAD
} TokenType;

typedef struct { TokenType type; char text[MAX_TEXT]; double num; int line; } Token;
typedef enum { TY_UNKNOWN, TY_NUMBER, TY_STRING, TY_BOOL } Type;
typedef enum { N_PROGRAM, N_LIST, N_ASSIGN, N_SHOW, N_IF, N_NUM, N_STR, N_BOOL, N_ID, N_BIN } NodeKind;

typedef struct Node {
    NodeKind kind;
    int line;
    Type type;
    char text[MAX_TEXT];
    double num;
    struct Node *a, *b, *c, *next;
} Node;

typedef struct { char name[MAX_TEXT]; Type type; int line; } Symbol;

static Token toks[MAX_TOKENS];
static int ntok = 0, pos = 0, had_error = 0, temp_id = 0, label_id = 0;
static Symbol syms[MAX_SYMBOLS];
static int nsym = 0;
static Node nodes[MAX_NODES];
static int nnode = 0;
static FILE *fast, *fsym, *ftac, *fc;

static const char *type_name(Type t) {
    switch (t) { case TY_NUMBER: return "number"; case TY_STRING: return "string"; case TY_BOOL: return "bool"; default: return "unknown"; }
}

static const char *tok_name(TokenType t) {
    switch (t) {
        case TOK_ID: return "identifier"; case TOK_NUM: return "number"; case TOK_STR: return "string";
        case TOK_TRUE: return "true"; case TOK_FALSE: return "false"; case TOK_SHOW: return "show";
        case TOK_IF: return "if"; case TOK_ELSE: return "else"; case TOK_PLUS: return "+";
        case TOK_MINUS: return "-"; case TOK_STAR: return "*"; case TOK_SLASH: return "/";
        case TOK_ASSIGN: return "="; case TOK_GT: return ">"; case TOK_LT: return "<";
        case TOK_GE: return ">="; case TOK_LE: return "<="; case TOK_EQ: return "==";
        case TOK_NE: return "!="; case TOK_LP: return "("; case TOK_RP: return ")";
        case TOK_LB: return "{"; case TOK_RB: return "}"; case TOK_SEMI: return ";";
        case TOK_EOF: return "end of file"; default: return "bad token";
    }
}

static void diag(const char *phase, int line, const char *fmt, ...) {
    va_list ap;
    had_error = 1;
    fprintf(stderr, "[%s error] line %d: ", phase, line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Cannot open %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    char *buf = (char *)calloc((size_t)n + 1, 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, (size_t)n, f);
    fclose(f);
    return buf;
}

static void addtok(TokenType t, const char *s, double num, int line) {
    if (ntok >= MAX_TOKENS) { diag("lexical", line, "too many tokens"); return; }
    toks[ntok].type = t; toks[ntok].num = num; toks[ntok].line = line;
    snprintf(toks[ntok].text, MAX_TEXT, "%s", s ? s : "");
    ntok++;
}

static void lex(const char *src) {
    int i = 0, line = 1;
    while (src[i]) {
        char c = src[i];
        if (c == '\n') { line++; i++; continue; }
        if (isspace((unsigned char)c)) { i++; continue; }
        if (c == '#') { while (src[i] && src[i] != '\n') i++; continue; }
        if (c == '/' && src[i+1] == '/') { while (src[i] && src[i] != '\n') i++; continue; }
        if (isalpha((unsigned char)c) || c == '_') {
            char b[MAX_TEXT]; int j = 0;
            while (isalnum((unsigned char)src[i]) || src[i] == '_') { if (j < MAX_TEXT-1) b[j++] = src[i]; i++; }
            b[j] = 0;
            if (!strcmp(b,"show")) addtok(TOK_SHOW,b,0,line);
            else if (!strcmp(b,"if")) addtok(TOK_IF,b,0,line);
            else if (!strcmp(b,"else")) addtok(TOK_ELSE,b,0,line);
            else if (!strcmp(b,"true")) addtok(TOK_TRUE,b,0,line);
            else if (!strcmp(b,"false")) addtok(TOK_FALSE,b,0,line);
            else addtok(TOK_ID,b,0,line);
            continue;
        }
        if (isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)src[i+1]))) {
            char b[MAX_TEXT]; int j = 0, dots = 0;
            while (isdigit((unsigned char)src[i]) || src[i] == '.') { if (src[i]=='.') dots++; if (j < MAX_TEXT-1) b[j++] = src[i]; i++; }
            b[j] = 0;
            if (dots > 1) diag("lexical", line, "malformed number '%s'", b);
            addtok(TOK_NUM, b, strtod(b, NULL), line);
            continue;
        }
        if (c == '"') {
            char b[MAX_TEXT]; int j = 0; i++;
            while (src[i] && src[i] != '"') {
                if (src[i] == '\n') { diag("lexical", line, "unterminated string literal"); return; }
                if (src[i] == '\\' && src[i+1]) i++;
                if (j < MAX_TEXT-1) b[j++] = src[i];
                i++;
            }
            if (!src[i]) { diag("lexical", line, "unterminated string literal"); return; }
            i++; b[j] = 0; addtok(TOK_STR, b, 0, line); continue;
        }
        if (c == '>' && src[i+1] == '=') { addtok(TOK_GE, ">=",0,line); i+=2; continue; }
        if (c == '<' && src[i+1] == '=') { addtok(TOK_LE, "<=",0,line); i+=2; continue; }
        if (c == '=' && src[i+1] == '=') { addtok(TOK_EQ, "==",0,line); i+=2; continue; }
        if (c == '!' && src[i+1] == '=') { addtok(TOK_NE, "!=",0,line); i+=2; continue; }
        switch(c) {
            case '+': addtok(TOK_PLUS,"+",0,line); break; case '-': addtok(TOK_MINUS,"-",0,line); break;
            case '*': addtok(TOK_STAR,"*",0,line); break; case '/': addtok(TOK_SLASH,"/",0,line); break;
            case '=': addtok(TOK_ASSIGN,"=",0,line); break; case '>': addtok(TOK_GT,">",0,line); break;
            case '<': addtok(TOK_LT,"<",0,line); break; case '(': addtok(TOK_LP,"(",0,line); break;
            case ')': addtok(TOK_RP,")",0,line); break; case '{': addtok(TOK_LB,"{",0,line); break;
            case '}': addtok(TOK_RB,"}",0,line); break; case ';': addtok(TOK_SEMI,";",0,line); break;
            default: diag("lexical", line, "unknown character '%c'", c); addtok(TOK_BAD, "", 0, line); break;
        }
        i++;
    }
    addtok(TOK_EOF, "", 0, line);
}

static Node *new_node(NodeKind k, int line) {
    if (nnode >= MAX_NODES) { diag("internal", line, "too many AST nodes"); exit(2); }
    Node *n = &nodes[nnode++]; memset(n, 0, sizeof(*n)); n->kind = k; n->line = line; n->type = TY_UNKNOWN; return n;
}

static Token *peek(void) { return &toks[pos]; }
static int match(TokenType t) { if (peek()->type == t) { pos++; return 1; } return 0; }
static Token *expect(TokenType t) {
    Token *p = peek();
    if (p->type == t) { pos++; return p; }
    diag("syntax", p->line, "expected %s but found %s", tok_name(t), tok_name(p->type));
    return p;
}

static Node *parse_stmt_list(int until_rb);
static Node *parse_expr(void);
static Node *parse_stmt(void);

static Node *wrap_stmt(Node *s) {
    Node *l = new_node(N_LIST, s ? s->line : peek()->line);
    l->a = s;
    return l;
}

static Node *parse_block_or_stmt(void) {
    if (match(TOK_LB)) {
        Node *body = parse_stmt_list(1);
        expect(TOK_RB);
        return body;
    }
    return wrap_stmt(parse_stmt());
}

static Node *parse_primary(void) {
    Token *t = peek();
    if (match(TOK_NUM)) { Node *n = new_node(N_NUM, t->line); n->num = t->num; snprintf(n->text, MAX_TEXT, "%s", t->text); n->type = TY_NUMBER; return n; }
    if (match(TOK_STR)) { Node *n = new_node(N_STR, t->line); snprintf(n->text, MAX_TEXT, "%s", t->text); n->type = TY_STRING; return n; }
    if (match(TOK_TRUE) || match(TOK_FALSE)) { Node *n = new_node(N_BOOL, t->line); n->num = (t->type == TOK_TRUE); snprintf(n->text, MAX_TEXT, "%s", t->text); n->type = TY_BOOL; return n; }
    if (match(TOK_ID)) { Node *n = new_node(N_ID, t->line); snprintf(n->text, MAX_TEXT, "%s", t->text); return n; }
    if (match(TOK_LP)) { Node *e = parse_expr(); expect(TOK_RP); return e; }
    diag("syntax", t->line, "invalid expression near %s", tok_name(t->type)); pos++; return new_node(N_NUM, t->line);
}

static Node *parse_mul(void) {
    Node *l = parse_primary();
    while (peek()->type == TOK_STAR || peek()->type == TOK_SLASH) {
        Token *op = peek(); pos++;
        Node *n = new_node(N_BIN, op->line); snprintf(n->text, MAX_TEXT, "%s", op->text); n->a = l; n->b = parse_primary(); l = n;
    }
    return l;
}

static Node *parse_add(void) {
    Node *l = parse_mul();
    while (peek()->type == TOK_PLUS || peek()->type == TOK_MINUS) {
        Token *op = peek(); pos++;
        Node *n = new_node(N_BIN, op->line); snprintf(n->text, MAX_TEXT, "%s", op->text); n->a = l; n->b = parse_mul(); l = n;
    }
    return l;
}

static Node *parse_expr(void) {
    Node *l = parse_add();
    if (peek()->type == TOK_GT || peek()->type == TOK_LT || peek()->type == TOK_GE || peek()->type == TOK_LE || peek()->type == TOK_EQ || peek()->type == TOK_NE) {
        Token *op = peek(); pos++;
        Node *n = new_node(N_BIN, op->line); snprintf(n->text, MAX_TEXT, "%s", op->text); n->a = l; n->b = parse_add(); l = n;
    }
    return l;
}

static Node *parse_stmt(void) {
    Token *t = peek();
    if (match(TOK_ID)) {
        Token *id = t;
        if (peek()->type == TOK_ASSIGN) {
            pos++;
        }
        Node *as = new_node(N_ASSIGN, id->line); snprintf(as->text, MAX_TEXT, "%s", id->text); as->a = parse_expr(); expect(TOK_SEMI); return as;
    }
    if (match(TOK_SHOW)) {
        Node *s = new_node(N_SHOW, t->line);
        if (match(TOK_LP)) {
            s->a = parse_expr();
            expect(TOK_RP);
        } else {
            s->a = parse_expr();
        }
        expect(TOK_SEMI);
        return s;
    }
    if (match(TOK_IF)) {
        Node *n = new_node(N_IF, t->line);
        expect(TOK_LP); n->a = parse_expr(); expect(TOK_RP);
        n->b = parse_block_or_stmt();
        if (!match(TOK_ELSE)) diag("syntax", peek()->line, "if statement requires else block");
        n->c = parse_block_or_stmt();
        return n;
    }
    diag("syntax", t->line, "unexpected %s at start of statement", tok_name(t->type));
    pos++; return new_node(N_LIST, t->line);
}

static Node *parse_stmt_list(int until_rb) {
    Node *head = NULL, *tail = NULL;
    while (peek()->type != TOK_EOF && !(until_rb && peek()->type == TOK_RB)) {
        Node *s = parse_stmt();
        if (!head) head = tail = s; else { tail->next = s; tail = s; }
    }
    Node *l = new_node(N_LIST, head ? head->line : peek()->line); l->a = head; return l;
}

static Node *parse_program(void) {
    Node *p = new_node(N_PROGRAM, 1); p->a = parse_stmt_list(0); expect(TOK_EOF); return p;
}

static Symbol *find_sym(const char *name) { for (int i=0;i<nsym;i++) if (!strcmp(syms[i].name,name)) return &syms[i]; return NULL; }
static Symbol *put_sym(const char *name, Type ty, int line) {
    Symbol *s = find_sym(name);
    if (s) {
        if (s->type != ty) diag("semantic", line, "type mismatch for '%s': was %s, assigned %s", name, type_name(s->type), type_name(ty));
        return s;
    }
    if (nsym >= MAX_SYMBOLS) { diag("semantic", line, "symbol table full"); return NULL; }
    snprintf(syms[nsym].name, MAX_TEXT, "%s", name); syms[nsym].type = ty; syms[nsym].line = line; return &syms[nsym++];
}

static int is_cmp(const char *op) { return !strcmp(op,">")||!strcmp(op,"<")||!strcmp(op,">=")||!strcmp(op,"<=")||!strcmp(op,"==")||!strcmp(op,"!="); }

static Type sem_expr(Node *n) {
    if (!n) return TY_UNKNOWN;
    if (n->kind == N_NUM || n->kind == N_STR || n->kind == N_BOOL) return n->type;
    if (n->kind == N_ID) {
        Symbol *s = find_sym(n->text);
        if (!s) { diag("semantic", n->line, "undefined variable '%s'", n->text); return TY_UNKNOWN; }
        n->type = s->type; return n->type;
    }
    if (n->kind == N_BIN) {
        Type a = sem_expr(n->a), b = sem_expr(n->b);
        if (is_cmp(n->text)) {
            if (a == TY_UNKNOWN || b == TY_UNKNOWN) return TY_UNKNOWN;
            if (a != b) diag("semantic", n->line, "invalid comparison between %s and %s", type_name(a), type_name(b));
            n->type = TY_BOOL; return TY_BOOL;
        }
        if (a != TY_NUMBER || b != TY_NUMBER) diag("semantic", n->line, "operator '%s' requires number operands", n->text);
        if (!strcmp(n->text,"/") && n->b->kind == N_NUM && n->b->num == 0) diag("semantic", n->line, "division by zero constant");
        n->type = TY_NUMBER; return TY_NUMBER;
    }
    return TY_UNKNOWN;
}

static void sem_list(Node *l) {
    for (Node *s = l ? l->a : NULL; s; s = s->next) {
        if (s->kind == N_ASSIGN) { Type t = sem_expr(s->a); if (t != TY_UNKNOWN) put_sym(s->text, t, s->line); }
        else if (s->kind == N_SHOW) sem_expr(s->a);
        else if (s->kind == N_IF) { Type t = sem_expr(s->a); if (t != TY_BOOL) diag("semantic", s->line, "if condition must be bool comparison"); sem_list(s->b); sem_list(s->c); }
    }
}

static void ast_indent(int d) { for (int i=0;i<d;i++) fprintf(fast, "  "); }
static void ast_print(Node *n, int d) {
    if (!n) return;
    ast_indent(d);
    switch(n->kind) {
        case N_PROGRAM: fprintf(fast,"Program\n"); ast_print(n->a,d+1); break;
        case N_LIST: fprintf(fast,"StatementList\n"); for (Node *s=n->a;s;s=s->next) ast_print(s,d+1); break;
        case N_ASSIGN: fprintf(fast,"Assign %s\n",n->text); ast_print(n->a,d+1); break;
        case N_SHOW: fprintf(fast,"Show\n"); ast_print(n->a,d+1); break;
        case N_IF: fprintf(fast,"IfElse\n"); ast_indent(d+1); fprintf(fast,"Condition\n"); ast_print(n->a,d+2); ast_indent(d+1); fprintf(fast,"Then\n"); ast_print(n->b,d+2); ast_indent(d+1); fprintf(fast,"Else\n"); ast_print(n->c,d+2); break;
        case N_NUM: fprintf(fast,"Number %g\n",n->num); break;
        case N_STR: fprintf(fast,"String \"%s\"\n",n->text); break;
        case N_BOOL: fprintf(fast,"Bool %s\n",n->text); break;
        case N_ID: fprintf(fast,"Identifier %s\n",n->text); break;
        case N_BIN: fprintf(fast,"Binary %s : %s\n",n->text,type_name(n->type)); ast_print(n->a,d+1); ast_print(n->b,d+1); break;
    }
}

static void symbol_print(void) {
    fprintf(fsym, "Name,Type,FirstLine\n");
    for (int i=0;i<nsym;i++) fprintf(fsym, "%s,%s,%d\n", syms[i].name, type_name(syms[i].type), syms[i].line);
}

typedef struct { char name[MAX_TEXT]; Type type; } Place;
static Place place(const char *n, Type t) { Place p; snprintf(p.name, MAX_TEXT, "%s", n); p.type=t; return p; }

static Place gen_expr(Node *n) {
    char b[MAX_TEXT];
    if (n->kind == N_NUM) { snprintf(b, sizeof(b), "%g", n->num); return place(b, TY_NUMBER); }
    if (n->kind == N_STR) { snprintf(b, sizeof(b), "\"%.240s\"", n->text); return place(b, TY_STRING); }
    if (n->kind == N_BOOL) return place(n->num ? "true" : "false", TY_BOOL);
    if (n->kind == N_ID) return place(n->text, n->type);
    Place a = gen_expr(n->a), c = gen_expr(n->b);
    snprintf(b, sizeof(b), "t%d", ++temp_id);
    fprintf(ftac, "%s = %s %s %s\n", b, a.name, n->text, c.name);
    return place(b, n->type);
}

static void gen_list(Node *l) {
    for (Node *s = l ? l->a : NULL; s; s=s->next) {
        if (s->kind == N_ASSIGN) { Place e = gen_expr(s->a); fprintf(ftac, "%s = %s\n", s->text, e.name); }
        else if (s->kind == N_SHOW) { Place e = gen_expr(s->a); fprintf(ftac, "param %s\ncall show\n", e.name); }
        else if (s->kind == N_IF) {
            int l1 = ++label_id, l2 = ++label_id; Place cond = gen_expr(s->a);
            fprintf(ftac, "ifFalse %s goto L%d\n", cond.name, l1);
            gen_list(s->b); fprintf(ftac, "goto L%d\nL%d:\n", l2, l1);
            gen_list(s->c); fprintf(ftac, "L%d:\n", l2);
        }
    }
}

static void c_escape(FILE *f, const char *s) { for (; *s; s++) { if (*s == '"' || *s == '\\') fputc('\\', f); fputc(*s, f); } }
static void c_expr(Node *n) {
    if (n->kind == N_NUM) fprintf(fc, "%g", n->num);
    else if (n->kind == N_STR) { fprintf(fc, "\""); c_escape(fc,n->text); fprintf(fc, "\""); }
    else if (n->kind == N_BOOL) fprintf(fc, "%d", n->num ? 1 : 0);
    else if (n->kind == N_ID) fprintf(fc, "%s", n->text);
    else if (n->kind == N_BIN) {
        if (!strcmp(n->text,"/")) {
            fprintf(fc, "rl_safe_div("); c_expr(n->a); fprintf(fc, ", "); c_expr(n->b); fprintf(fc, ")");
        } else {
            fprintf(fc, "("); c_expr(n->a); fprintf(fc, " %s ", n->text); c_expr(n->b); fprintf(fc, ")");
        }
    }
}

static void c_decls(void) {
    for (int i=0;i<nsym;i++) {
        if (syms[i].type == TY_STRING) fprintf(fc, "    const char *%s = \"\";\n", syms[i].name);
        else if (syms[i].type == TY_BOOL) fprintf(fc, "    int %s = 0;\n", syms[i].name);
        else fprintf(fc, "    double %s = 0;\n", syms[i].name);
    }
}

static void c_list(Node *l, int d) {
    for (Node *s = l ? l->a : NULL; s; s=s->next) {
        for (int i=0;i<d;i++) fprintf(fc,"    ");
        if (s->kind == N_ASSIGN) { fprintf(fc, "%s = ", s->text); c_expr(s->a); fprintf(fc, ";\n"); }
        else if (s->kind == N_SHOW) {
            Type t = s->a->type;
            if (t == TY_STRING) { fprintf(fc, "printf(\"%%s\\n\", "); c_expr(s->a); fprintf(fc, ");\n"); }
            else if (t == TY_BOOL) { fprintf(fc, "printf(\"%%s\\n\", ("); c_expr(s->a); fprintf(fc, ") ? \"true\" : \"false\");\n"); }
            else { fprintf(fc, "printf(\"%%.2f\\n\", "); c_expr(s->a); fprintf(fc, ");\n"); }
        } else if (s->kind == N_IF) {
            fprintf(fc, "if ("); c_expr(s->a); fprintf(fc, ") {\n"); c_list(s->b,d+1);
            for (int i=0;i<d;i++) fprintf(fc,"    ");
            fprintf(fc, "} else {\n"); c_list(s->c,d+1);
            for (int i=0;i<d;i++) fprintf(fc,"    ");
            fprintf(fc, "}\n");
        }
    }
}

static void gen_c(Node *p) {
    fprintf(fc, "#include <stdio.h>\n#include <stdlib.h>\n\n");
    fprintf(fc, "static double rl_safe_div(double a, double b) { if (b == 0) { fprintf(stderr, \"runtime error: division by zero\\n\"); exit(2); } return a / b; }\n\n");
    fprintf(fc, "int main(void) {\n"); c_decls(); c_list(p->a,1); fprintf(fc, "    return 0;\n}\n");
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: resultlangc source.rl [outdir]\n"); return 2; }
    const char *out = argc >= 3 ? argv[2] : "generated";
    char path[512];
    char *src = read_file(argv[1]); if (!src) return 2;
    lex(src);
    if (had_error) return 1;
    Node *program = parse_program();
    if (had_error) return 1;
    sem_list(program->a);
    if (had_error) return 1;
    snprintf(path,sizeof(path),"%s/ast.txt",out); fast=fopen(path,"w"); ast_print(program,0); fclose(fast);
    snprintf(path,sizeof(path),"%s/symbol_table.csv",out); fsym=fopen(path,"w"); symbol_print(); fclose(fsym);
    snprintf(path,sizeof(path),"%s/intermediate.tac",out); ftac=fopen(path,"w"); gen_list(program->a); fclose(ftac);
    snprintf(path,sizeof(path),"%s/generated.c",out); fc=fopen(path,"w"); gen_c(program); fclose(fc);
    printf("ResultLang compilation successful. Outputs written to %s\n", out);
    free(src);
    return 0;
}
