#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Interprete de C--:
 * 1) Lexer: convierte texto en tokens.
 * 2) Parser + evaluador: analiza sentencias/expresiones y ejecuta en el momento.
 * 3) Tabla de simbolos: guarda variables, tipo e inicializacion.
 */

typedef enum {
    TOK_EOF,
    TOK_IDENTIFIER,
    TOK_NUMBER,
    TOK_STRING,
    TOK_SEMICOLON,
    TOK_ASSIGN,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_KW_INT,
    TOK_KW_FLOAT,
    TOK_KW_STRING,
    TOK_KW_BOOL,
    TOK_KW_PRINT,
    TOK_KW_TRUE,
    TOK_KW_FALSE
} TokenType;

/* Token individual con su texto y posicion para mensajes de error. */
typedef struct {
    TokenType type;
    char *lexeme;
    int line;
    int col;
} Token;

typedef struct {
    const char *src;
    size_t pos;
    int line;
    int col;
    Token current;
} Lexer;

/* Representacion de valores en tiempo de ejecucion. */
typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_STRING,
    VAL_BOOL
} ValueType;

typedef struct {
    ValueType type;
    union {
        int as_int;
        double as_float;
        char *as_string;
        int as_bool;
    } data;
} Value;

/* Variable en tabla de simbolos: nombre, valor y bandera de inicializacion. */
typedef struct {
    char *name;
    Value value;
    int initialized;
} Variable;

typedef struct {
    Variable *items;
    size_t count;
    size_t capacity;
} Scope;

typedef struct {
    Scope *scopes;
    size_t count;
    size_t capacity;
} SymbolTable;

typedef struct {
    Lexer lexer;
    SymbolTable symbols;
} Parser;

/* Error fatal con ubicacion (linea/columna) y finalizacion inmediata. */
static void fatal_at(int line, int col, const char *msg) {
    fprintf(stderr, "Error (%d:%d): %s\n", line, col, msg);
    exit(1);
}

/* Utilidades seguras para duplicar cadenas del codigo fuente. */
static char *strdup_cmm(const char *s) {
    size_t len = strlen(s);
    char *copy = (char *)malloc(len + 1);
    if (!copy) {
        fprintf(stderr, "Error: memoria insuficiente.\n");
        exit(1);
    }
    memcpy(copy, s, len + 1);
    return copy;
}

static char *substrdup(const char *s, size_t start, size_t end) {
    if (end < start) {
        return strdup_cmm("");
    }
    size_t len = end - start;
    char *out = (char *)malloc(len + 1);
    if (!out) {
        fprintf(stderr, "Error: memoria insuficiente.\n");
        exit(1);
    }
    memcpy(out, s + start, len);
    out[len] = '\0';
    return out;
}

static void free_value(Value *v) {
    if (v->type == VAL_STRING && v->data.as_string) {
        free(v->data.as_string);
        v->data.as_string = NULL;
    }
}

/* ---------------- Tabla de simbolos ---------------- */
static void symbol_table_init(SymbolTable *table) {
    table->scopes = NULL;
    table->count = 0;
    table->capacity = 0;

    /* Crea el ambito global por defecto. */
    if (table->count == table->capacity) {
        size_t new_capacity = table->capacity == 0 ? 4 : table->capacity * 2;
        Scope *new_scopes = (Scope *)realloc(table->scopes, new_capacity * sizeof(Scope));
        if (!new_scopes) {
            fprintf(stderr, "Error: memoria insuficiente.\n");
            exit(1);
        }
        table->scopes = new_scopes;
        table->capacity = new_capacity;
    }

    Scope *global = &table->scopes[table->count++];
    global->items = NULL;
    global->count = 0;
    global->capacity = 0;
}

static Scope *symbol_table_current_scope(SymbolTable *table) {
    if (table->count == 0) {
        return NULL;
    }
    return &table->scopes[table->count - 1];
}

static Variable *scope_find(Scope *scope, const char *name) {
    for (size_t i = 0; i < scope->count; i++) {
        if (strcmp(scope->items[i].name, name) == 0) {
            return &scope->items[i];
        }
    }
    return NULL;
}

static void symbol_table_push_scope(SymbolTable *table) {
    if (table->count == table->capacity) {
        size_t new_capacity = table->capacity == 0 ? 4 : table->capacity * 2;
        Scope *new_scopes = (Scope *)realloc(table->scopes, new_capacity * sizeof(Scope));
        if (!new_scopes) {
            fprintf(stderr, "Error: memoria insuficiente.\n");
            exit(1);
        }
        table->scopes = new_scopes;
        table->capacity = new_capacity;
    }

    Scope *scope = &table->scopes[table->count++];
    scope->items = NULL;
    scope->count = 0;
    scope->capacity = 0;
}

static void symbol_table_pop_scope(SymbolTable *table) {
    if (table->count == 0) {
        return;
    }

    Scope *scope = &table->scopes[table->count - 1];
    for (size_t i = 0; i < scope->count; i++) {
        free(scope->items[i].name);
        free_value(&scope->items[i].value);
    }
    free(scope->items);
    table->count--;
}

static Variable *symbol_table_lookup(SymbolTable *table, const char *name) {
    for (size_t i = table->count; i > 0; i--) {
        Scope *scope = &table->scopes[i - 1];
        Variable *v = scope_find(scope, name);
        if (v) {
            return v;
        }
    }
    return NULL;
}

static void symbol_table_add(SymbolTable *table, const char *name, ValueType type, int line, int col) {
    Scope *scope = symbol_table_current_scope(table);
    if (!scope) {
        fatal_at(line, col, "no hay ambito activo");
    }

    if (scope_find(scope, name)) {
        fatal_at(line, col, "variable redeclarada en el mismo ambito");
    }

    if (scope->count == scope->capacity) {
        size_t new_capacity = scope->capacity == 0 ? 8 : scope->capacity * 2;
        Variable *new_items = (Variable *)realloc(scope->items, new_capacity * sizeof(Variable));
        if (!new_items) {
            fprintf(stderr, "Error: memoria insuficiente.\n");
            exit(1);
        }
        scope->items = new_items;
        scope->capacity = new_capacity;
    }

    Variable *v = &scope->items[scope->count++];
    v->name = strdup_cmm(name);
    v->value.type = type;
    v->initialized = 0;
    if (type == VAL_STRING) {
        v->value.data.as_string = NULL;
    } else if (type == VAL_INT) {
        v->value.data.as_int = 0;
    } else if (type == VAL_FLOAT) {
        v->value.data.as_float = 0.0;
    } else {
        v->value.data.as_bool = 0;
    }
}

static void symbol_table_free(SymbolTable *table) {
    while (table->count > 0) {
        symbol_table_pop_scope(table);
    }
    free(table->scopes);
}

/* ---------------- Lexer (analisis lexico) ---------------- */
static int lexer_is_at_end(Lexer *lexer) {
    return lexer->src[lexer->pos] == '\0';
}

static char lexer_peek(Lexer *lexer) {
    return lexer->src[lexer->pos];
}

static char lexer_peek_next(Lexer *lexer) {
    if (lexer->src[lexer->pos] == '\0') {
        return '\0';
    }
    return lexer->src[lexer->pos + 1];
}

static char lexer_advance(Lexer *lexer) {
    char c = lexer->src[lexer->pos++];
    if (c == '\n') {
        lexer->line++;
        lexer->col = 1;
    } else {
        lexer->col++;
    }
    return c;
}

/* Consume espacios y comentarios para dejar el siguiente token listo. */
static void skip_whitespace_and_comments(Lexer *lexer) {
    for (;;) {
        char c = lexer_peek(lexer);

        while (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
            lexer_advance(lexer);
            c = lexer_peek(lexer);
        }

        if (c == '/' && lexer_peek_next(lexer) == '/') {
            while (!lexer_is_at_end(lexer) && lexer_peek(lexer) != '\n') {
                lexer_advance(lexer);
            }
            continue;
        }

        if (c == '/' && lexer_peek_next(lexer) == '*') {
            lexer_advance(lexer);
            lexer_advance(lexer);
            while (!lexer_is_at_end(lexer)) {
                if (lexer_peek(lexer) == '*' && lexer_peek_next(lexer) == '/') {
                    lexer_advance(lexer);
                    lexer_advance(lexer);
                    break;
                }
                lexer_advance(lexer);
            }
            if (lexer_is_at_end(lexer)) {
                fatal_at(lexer->line, lexer->col, "comentario multilinea sin cerrar");
            }
            continue;
        }

        break;
    }
}

static Token make_token(Lexer *lexer, TokenType type, char *lexeme, int line, int col) {
    Token t;
    t.type = type;
    t.lexeme = lexeme;
    t.line = line;
    t.col = col;
    (void)lexer;
    return t;
}

/* Reconoce palabras reservadas; si no coincide, es identificador. */
static TokenType keyword_type(const char *text) {
    if (strcmp(text, "int") == 0) return TOK_KW_INT;
    if (strcmp(text, "float") == 0) return TOK_KW_FLOAT;
    if (strcmp(text, "string") == 0) return TOK_KW_STRING;
    if (strcmp(text, "bool") == 0) return TOK_KW_BOOL;
    if (strcmp(text, "print") == 0) return TOK_KW_PRINT;
    if (strcmp(text, "true") == 0) return TOK_KW_TRUE;
    if (strcmp(text, "false") == 0) return TOK_KW_FALSE;
    return TOK_IDENTIFIER;
}

/* Obtiene el siguiente token del flujo de entrada. */
static Token lexer_next_token(Lexer *lexer) {
    skip_whitespace_and_comments(lexer);

    int line = lexer->line;
    int col = lexer->col;

    if (lexer_is_at_end(lexer)) {
        return make_token(lexer, TOK_EOF, strdup_cmm(""), line, col);
    }

    char c = lexer_advance(lexer);

    if (isalpha((unsigned char)c) || c == '_') {
        size_t start = lexer->pos - 1;
        while (isalnum((unsigned char)lexer_peek(lexer)) || lexer_peek(lexer) == '_') {
            lexer_advance(lexer);
        }
        char *text = substrdup(lexer->src, start, lexer->pos);
        TokenType type = keyword_type(text);
        return make_token(lexer, type, text, line, col);
    }

    if (isdigit((unsigned char)c)) {
        size_t start = lexer->pos - 1;
        while (isdigit((unsigned char)lexer_peek(lexer))) {
            lexer_advance(lexer);
        }
        if (lexer_peek(lexer) == '.' && isdigit((unsigned char)lexer_peek_next(lexer))) {
            lexer_advance(lexer);
            while (isdigit((unsigned char)lexer_peek(lexer))) {
                lexer_advance(lexer);
            }
        }
        char *num = substrdup(lexer->src, start, lexer->pos);
        return make_token(lexer, TOK_NUMBER, num, line, col);
    }

    if (c == '"') {
        size_t start = lexer->pos;
        while (!lexer_is_at_end(lexer) && lexer_peek(lexer) != '"') {
            if (lexer_peek(lexer) == '\n') {
                fatal_at(line, col, "cadena sin cerrar");
            }
            lexer_advance(lexer);
        }
        if (lexer_is_at_end(lexer)) {
            fatal_at(line, col, "cadena sin cerrar");
        }
        char *text = substrdup(lexer->src, start, lexer->pos);
        lexer_advance(lexer);
        return make_token(lexer, TOK_STRING, text, line, col);
    }

    switch (c) {
        case ';': return make_token(lexer, TOK_SEMICOLON, strdup_cmm(";"), line, col);
        case '=': return make_token(lexer, TOK_ASSIGN, strdup_cmm("="), line, col);
        case '(': return make_token(lexer, TOK_LPAREN, strdup_cmm("("), line, col);
        case ')': return make_token(lexer, TOK_RPAREN, strdup_cmm(")"), line, col);
        case '+': return make_token(lexer, TOK_PLUS, strdup_cmm("+"), line, col);
        case '-': return make_token(lexer, TOK_MINUS, strdup_cmm("-"), line, col);
        case '*': return make_token(lexer, TOK_STAR, strdup_cmm("*"), line, col);
        case '/': return make_token(lexer, TOK_SLASH, strdup_cmm("/"), line, col);
        case '{': return make_token(lexer, TOK_LBRACE, strdup_cmm("{"), line, col);
        case '}': return make_token(lexer, TOK_RBRACE, strdup_cmm("}"), line, col);
        default: fatal_at(line, col, "caracter no valido");
    }

    return make_token(lexer, TOK_EOF, strdup_cmm(""), line, col);
}

/* ---------------- Parser/evaluador ---------------- */
static void token_free(Token *t) {
    free(t->lexeme);
    t->lexeme = NULL;
}

static void parser_init(Parser *parser, const char *source) {
    parser->lexer.src = source;
    parser->lexer.pos = 0;
    parser->lexer.line = 1;
    parser->lexer.col = 1;
    symbol_table_init(&parser->symbols);
    parser->lexer.current = lexer_next_token(&parser->lexer);
}

/* Avanza un token liberando memoria del token anterior. */
static void parser_advance(Parser *parser) {
    token_free(&parser->lexer.current);
    parser->lexer.current = lexer_next_token(&parser->lexer);
}

static int parser_match(Parser *parser, TokenType type) {
    if (parser->lexer.current.type == type) {
        parser_advance(parser);
        return 1;
    }
    return 0;
}

static void parser_expect(Parser *parser, TokenType type, const char *msg) {
    if (parser->lexer.current.type != type) {
        fatal_at(parser->lexer.current.line, parser->lexer.current.col, msg);
    }
    parser_advance(parser);
}

/* Copia profunda de valores para evitar aliasing de strings. */
static Value value_copy(Value v) {
    Value out;
    out.type = v.type;
    if (v.type == VAL_STRING) {
        out.data.as_string = strdup_cmm(v.data.as_string ? v.data.as_string : "");
    } else if (v.type == VAL_INT) {
        out.data.as_int = v.data.as_int;
    } else if (v.type == VAL_FLOAT) {
        out.data.as_float = v.data.as_float;
    } else {
        out.data.as_bool = v.data.as_bool;
    }
    return out;
}

static Value make_int(int x) {
    Value v;
    v.type = VAL_INT;
    v.data.as_int = x;
    return v;
}

static Value make_float(double x) {
    Value v;
    v.type = VAL_FLOAT;
    v.data.as_float = x;
    return v;
}

static Value make_bool(int x) {
    Value v;
    v.type = VAL_BOOL;
    v.data.as_bool = x ? 1 : 0;
    return v;
}

static Value make_string(const char *s) {
    Value v;
    v.type = VAL_STRING;
    v.data.as_string = strdup_cmm(s);
    return v;
}

static int is_numeric(ValueType t) {
    return t == VAL_INT || t == VAL_FLOAT;
}

static Value parse_expression(Parser *parser);

/* Factor: literales, variables, parentesis y menos unario. */
static Value parse_factor(Parser *parser) {
    Token t = parser->lexer.current;

    if (t.type == TOK_NUMBER) {
        char *lex = strdup_cmm(t.lexeme);
        parser_advance(parser);
        if (strchr(lex, '.')) {
            Value v = make_float(strtod(lex, NULL));
            free(lex);
            return v;
        }
        Value v = make_int((int)strtol(lex, NULL, 10));
        free(lex);
        return v;
    }

    if (t.type == TOK_STRING) {
        char *lex = strdup_cmm(t.lexeme);
        parser_advance(parser);
        Value v = make_string(lex);
        free(lex);
        return v;
    }

    if (parser_match(parser, TOK_KW_TRUE)) {
        return make_bool(1);
    }

    if (parser_match(parser, TOK_KW_FALSE)) {
        return make_bool(0);
    }

    if (t.type == TOK_IDENTIFIER) {
        char *name = strdup_cmm(t.lexeme);
        parser_advance(parser);
        Variable *var = symbol_table_lookup(&parser->symbols, name);
        if (!var) {
            free(name);
            fatal_at(t.line, t.col, "uso de variable no declarada");
        }
        if (!var->initialized) {
            free(name);
            fatal_at(t.line, t.col, "uso de variable sin inicializar");
        }
        Value v = value_copy(var->value);
        free(name);
        return v;
    }

    if (parser_match(parser, TOK_LPAREN)) {
        Value inner = parse_expression(parser);
        parser_expect(parser, TOK_RPAREN, "se esperaba ')' ");
        return inner;
    }

    if (parser_match(parser, TOK_MINUS)) {
        Value right = parse_factor(parser);
        if (right.type == VAL_INT) {
            right.data.as_int = -right.data.as_int;
            return right;
        }
        if (right.type == VAL_FLOAT) {
            right.data.as_float = -right.data.as_float;
            return right;
        }
        free_value(&right);
        fatal_at(t.line, t.col, "operador unario '-' solo admite numeros");
    }

    fatal_at(t.line, t.col, "expresion invalida");
    return make_int(0);
}

/* Termino: resuelve '*' y '/' con mayor precedencia. */
static Value parse_term(Parser *parser) {
    Value left = parse_factor(parser);

    while (parser->lexer.current.type == TOK_STAR || parser->lexer.current.type == TOK_SLASH) {
        TokenType op = parser->lexer.current.type;
        int op_line = parser->lexer.current.line;
        int op_col = parser->lexer.current.col;
        parser_advance(parser);
        Value right = parse_factor(parser);

        if (!is_numeric(left.type) || !is_numeric(right.type)) {
            free_value(&left);
            free_value(&right);
            fatal_at(op_line, op_col, "operacion aritmetica invalida");
        }

        int use_float = (left.type == VAL_FLOAT || right.type == VAL_FLOAT);
        if (use_float) {
            double a = (left.type == VAL_FLOAT) ? left.data.as_float : (double)left.data.as_int;
            double b = (right.type == VAL_FLOAT) ? right.data.as_float : (double)right.data.as_int;
            free_value(&left);
            free_value(&right);
            if (op == TOK_SLASH && b == 0.0) {
                fatal_at(op_line, op_col, "division por cero");
            }
            left = (op == TOK_STAR) ? make_float(a * b) : make_float(a / b);
        } else {
            int a = left.data.as_int;
            int b = right.data.as_int;
            free_value(&left);
            free_value(&right);
            if (op == TOK_SLASH && b == 0) {
                fatal_at(op_line, op_col, "division por cero");
            }
            left = (op == TOK_STAR) ? make_int(a * b) : make_int(a / b);
        }
    }

    return left;
}

/* Expresion: resuelve '+' y '-' y la concatenacion de strings. */
static Value parse_expression(Parser *parser) {
    Value left = parse_term(parser);

    while (parser->lexer.current.type == TOK_PLUS || parser->lexer.current.type == TOK_MINUS) {
        TokenType op = parser->lexer.current.type;
        int op_line = parser->lexer.current.line;
        int op_col = parser->lexer.current.col;
        parser_advance(parser);
        Value right = parse_term(parser);

        if (op == TOK_PLUS && (left.type == VAL_STRING || right.type == VAL_STRING)) {
            const char *a = (left.type == VAL_STRING) ? left.data.as_string : NULL;
            const char *b = (right.type == VAL_STRING) ? right.data.as_string : NULL;
            char num_buf_a[64];
            char num_buf_b[64];

            if (!a) {
                if (left.type == VAL_INT) {
                    snprintf(num_buf_a, sizeof(num_buf_a), "%d", left.data.as_int);
                } else if (left.type == VAL_FLOAT) {
                    snprintf(num_buf_a, sizeof(num_buf_a), "%g", left.data.as_float);
                } else if (left.type == VAL_BOOL) {
                    snprintf(num_buf_a, sizeof(num_buf_a), "%s", left.data.as_bool ? "true" : "false");
                }
                a = num_buf_a;
            }

            if (!b) {
                if (right.type == VAL_INT) {
                    snprintf(num_buf_b, sizeof(num_buf_b), "%d", right.data.as_int);
                } else if (right.type == VAL_FLOAT) {
                    snprintf(num_buf_b, sizeof(num_buf_b), "%g", right.data.as_float);
                } else if (right.type == VAL_BOOL) {
                    snprintf(num_buf_b, sizeof(num_buf_b), "%s", right.data.as_bool ? "true" : "false");
                }
                b = num_buf_b;
            }

            size_t len = strlen(a) + strlen(b);
            char *joined = (char *)malloc(len + 1);
            if (!joined) {
                fprintf(stderr, "Error: memoria insuficiente.\n");
                exit(1);
            }
            strcpy(joined, a);
            strcat(joined, b);

            free_value(&left);
            free_value(&right);
            left.type = VAL_STRING;
            left.data.as_string = joined;
            continue;
        }

        if (!is_numeric(left.type) || !is_numeric(right.type)) {
            free_value(&left);
            free_value(&right);
            fatal_at(op_line, op_col, "operacion aritmetica invalida");
        }

        int use_float = (left.type == VAL_FLOAT || right.type == VAL_FLOAT);
        if (use_float) {
            double a = (left.type == VAL_FLOAT) ? left.data.as_float : (double)left.data.as_int;
            double b = (right.type == VAL_FLOAT) ? right.data.as_float : (double)right.data.as_int;
            free_value(&left);
            free_value(&right);
            left = (op == TOK_PLUS) ? make_float(a + b) : make_float(a - b);
        } else {
            int a = left.data.as_int;
            int b = right.data.as_int;
            free_value(&left);
            free_value(&right);
            left = (op == TOK_PLUS) ? make_int(a + b) : make_int(a - b);
        }
    }

    return left;
}

static ValueType type_from_token(TokenType t, int line, int col) {
    switch (t) {
        case TOK_KW_INT: return VAL_INT;
        case TOK_KW_FLOAT: return VAL_FLOAT;
        case TOK_KW_STRING: return VAL_STRING;
        case TOK_KW_BOOL: return VAL_BOOL;
        default: fatal_at(line, col, "tipo de dato invalido");
    }
    return VAL_INT;
}

static void parse_statement(Parser *parser);

/* Bloque: '{' {sentencias} '}' con apertura/cierre de ambito. */
static void parse_block(Parser *parser) {
    parser_expect(parser, TOK_LBRACE, "se esperaba '{'");
    symbol_table_push_scope(&parser->symbols);

    while (parser->lexer.current.type != TOK_RBRACE && parser->lexer.current.type != TOK_EOF) {
        parse_statement(parser);
    }

    if (parser->lexer.current.type == TOK_EOF) {
        fatal_at(parser->lexer.current.line, parser->lexer.current.col, "se esperaba '}' para cerrar bloque");
    }

    parser_expect(parser, TOK_RBRACE, "se esperaba '}'");
    symbol_table_pop_scope(&parser->symbols);
}

/* Valida tipos y asigna un valor a la variable destino. */
static void assign_to_variable(Variable *var, Value value, int line, int col) {
    if (var->value.type == VAL_INT) {
        if (value.type != VAL_INT) {
            free_value(&value);
            fatal_at(line, col, "asignacion invalida: se esperaba int");
        }
        if (var->initialized) free_value(&var->value);
        var->value = value;
        var->initialized = 1;
        return;
    }

    if (var->value.type == VAL_FLOAT) {
        Value converted;
        if (value.type == VAL_FLOAT) {
            converted = value;
        } else if (value.type == VAL_INT) {
            converted = make_float((double)value.data.as_int);
            free_value(&value);
        } else {
            free_value(&value);
            fatal_at(line, col, "asignacion invalida: se esperaba float");
        }
        if (var->initialized) free_value(&var->value);
        var->value = converted;
        var->initialized = 1;
        return;
    }

    if (var->value.type == VAL_STRING) {
        if (value.type != VAL_STRING) {
            free_value(&value);
            fatal_at(line, col, "asignacion invalida: se esperaba string");
        }
        if (var->initialized) free_value(&var->value);
        var->value = value;
        var->initialized = 1;
        return;
    }

    if (var->value.type == VAL_BOOL) {
        if (value.type != VAL_BOOL) {
            free_value(&value);
            fatal_at(line, col, "asignacion invalida: se esperaba bool");
        }
        if (var->initialized) free_value(&var->value);
        var->value = value;
        var->initialized = 1;
        return;
    }
}

/* Declaracion: tipo + identificador [+ inicializacion]. */
static void parse_declaration(Parser *parser) {
    Token type_token = parser->lexer.current;
    ValueType declared_type = type_from_token(type_token.type, type_token.line, type_token.col);
    parser_advance(parser);

    if (parser->lexer.current.type != TOK_IDENTIFIER) {
        fatal_at(parser->lexer.current.line, parser->lexer.current.col, "se esperaba identificador");
    }

    Token name_token = parser->lexer.current;
    char *name_copy = strdup_cmm(name_token.lexeme);
    parser_advance(parser);

    symbol_table_add(&parser->symbols, name_copy, declared_type, name_token.line, name_token.col);
    Variable *var = symbol_table_lookup(&parser->symbols, name_copy);
    free(name_copy);

    if (parser_match(parser, TOK_ASSIGN)) {
        Value expr = parse_expression(parser);
        assign_to_variable(var, expr, name_token.line, name_token.col);
    }

    parser_expect(parser, TOK_SEMICOLON, "se esperaba ';' al final de declaracion");
}

/* Asignacion: identificador '=' expresion ';'. */
static void parse_assignment(Parser *parser) {
    Token name_token = parser->lexer.current;
    char *name_copy = strdup_cmm(name_token.lexeme);
    parser_advance(parser);
    parser_expect(parser, TOK_ASSIGN, "se esperaba '=' en asignacion");

    Variable *var = symbol_table_lookup(&parser->symbols, name_copy);
    if (!var) {
        free(name_copy);
        fatal_at(name_token.line, name_token.col, "asignacion a variable no declarada");
    }

    Value expr = parse_expression(parser);
    assign_to_variable(var, expr, name_token.line, name_token.col);
    parser_expect(parser, TOK_SEMICOLON, "se esperaba ';' al final de asignacion");
    free(name_copy);
}

/* print(expr); evalua la expresion y la muestra en stdout. */
static void parse_print(Parser *parser) {
    parser_expect(parser, TOK_KW_PRINT, "se esperaba 'print'");
    parser_expect(parser, TOK_LPAREN, "se esperaba '(' despues de print");
    Value v = parse_expression(parser);
    parser_expect(parser, TOK_RPAREN, "se esperaba ')' ");
    parser_expect(parser, TOK_SEMICOLON, "se esperaba ';' al final de print");

    if (v.type == VAL_INT) {
        printf("%d\n", v.data.as_int);
    } else if (v.type == VAL_FLOAT) {
        printf("%g\n", v.data.as_float);
    } else if (v.type == VAL_STRING) {
        printf("%s\n", v.data.as_string);
    } else {
        printf("%s\n", v.data.as_bool ? "true" : "false");
    }

    free_value(&v);
}

/* Dispatcher de sentencias soportadas en esta version. */
static void parse_statement(Parser *parser) {
    TokenType t = parser->lexer.current.type;

    if (t == TOK_LBRACE) {
        parse_block(parser);
        return;
    }

    if (t == TOK_KW_INT || t == TOK_KW_FLOAT || t == TOK_KW_STRING || t == TOK_KW_BOOL) {
        parse_declaration(parser);
        return;
    }

    if (t == TOK_IDENTIFIER) {
        parse_assignment(parser);
        return;
    }

    if (t == TOK_KW_PRINT) {
        parse_print(parser);
        return;
    }

    fatal_at(parser->lexer.current.line, parser->lexer.current.col, "sentencia no reconocida");
}

/* Ejecuta todo el programa de entrada hasta EOF. */
static void run_source(const char *source) {
    Parser parser;
    parser_init(&parser, source);

    while (parser.lexer.current.type != TOK_EOF) {
        parse_statement(&parser);
    }

    token_free(&parser.lexer.current);
    symbol_table_free(&parser.symbols);
}

/* Carga el archivo completo en memoria para ser interpretado. */
static char *read_file_content(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "No se pudo abrir el archivo: %s\n", path);
        exit(1);
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        fprintf(stderr, "No se pudo leer el archivo: %s\n", path);
        exit(1);
    }

    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        fprintf(stderr, "No se pudo leer el archivo: %s\n", path);
        exit(1);
    }
    rewind(f);

    char *buffer = (char *)malloc((size_t)size + 1);
    if (!buffer) {
        fclose(f);
        fprintf(stderr, "Error: memoria insuficiente.\n");
        exit(1);
    }

    size_t readn = fread(buffer, 1, (size_t)size, f);
    fclose(f);
    buffer[readn] = '\0';
    return buffer;
}

/* Punto de entrada: valida argumento, lee archivo y ejecuta. */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s archivo.cmm\n", argv[0]);
        return 1;
    }

    char *source = read_file_content(argv[1]);
    run_source(source);
    free(source);

    return 0;
}
