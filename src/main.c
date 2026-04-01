#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "interpreter.h"
#include "symbols.h"

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
    TOK_COMMA,
    TOK_COLON,
    TOK_ARROW,
    TOK_KW_INT,
    TOK_KW_FLOAT,
    TOK_KW_STRING,
    TOK_KW_BOOL,
    TOK_KW_PRINT,
    TOK_KW_TRUE,
    TOK_KW_FALSE,
    TOK_KW_FUNC,
    TOK_KW_RETURN
} TokenType;

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

typedef struct {
    Lexer lexer;
} Parser;

typedef struct {
    char **names;
    DataType *types;
    size_t count;
    size_t capacity;
} ParameterList;

typedef struct {
    ASTNode **items;
    size_t count;
    size_t capacity;
} ArgumentList;

static void fatal_at(int line, int col, const char *msg) {
    fprintf(stderr, "Error (%d:%d): %s\n", line, col, msg);
    exit(1);
}

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
    size_t len = end > start ? (end - start) : 0;
    char *out = (char *)malloc(len + 1);
    if (!out) {
        fprintf(stderr, "Error: memoria insuficiente.\n");
        exit(1);
    }
    memcpy(out, s + start, len);
    out[len] = '\0';
    return out;
}

static int lexer_is_at_end(Lexer *lexer) {
    return lexer->src[lexer->pos] == '\0';
}

static char lexer_peek(Lexer *lexer) {
    return lexer->src[lexer->pos];
}

static char lexer_peek_next(Lexer *lexer) {
    if (lexer_is_at_end(lexer)) {
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

static void skip_whitespace_and_comments(Lexer *lexer) {
    for (;;) {
        char c = lexer_peek(lexer);

        while (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
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

static Token make_token(TokenType type, char *lexeme, int line, int col) {
    Token t;
    t.type = type;
    t.lexeme = lexeme;
    t.line = line;
    t.col = col;
    return t;
}

static TokenType keyword_type(const char *text) {
    if (strcmp(text, "int") == 0) return TOK_KW_INT;
    if (strcmp(text, "float") == 0) return TOK_KW_FLOAT;
    if (strcmp(text, "string") == 0) return TOK_KW_STRING;
    if (strcmp(text, "bool") == 0) return TOK_KW_BOOL;
    if (strcmp(text, "print") == 0) return TOK_KW_PRINT;
    if (strcmp(text, "true") == 0) return TOK_KW_TRUE;
    if (strcmp(text, "false") == 0) return TOK_KW_FALSE;
    if (strcmp(text, "func") == 0) return TOK_KW_FUNC;
    if (strcmp(text, "return") == 0) return TOK_KW_RETURN;
    return TOK_IDENTIFIER;
}

static Token lexer_next_token(Lexer *lexer) {
    skip_whitespace_and_comments(lexer);

    int line = lexer->line;
    int col = lexer->col;

    if (lexer_is_at_end(lexer)) {
        return make_token(TOK_EOF, strdup_cmm(""), line, col);
    }

    char c = lexer_advance(lexer);

    if (isalpha((unsigned char)c) || c == '_') {
        size_t start = lexer->pos - 1;
        while (isalnum((unsigned char)lexer_peek(lexer)) || lexer_peek(lexer) == '_') {
            lexer_advance(lexer);
        }
        char *text = substrdup(lexer->src, start, lexer->pos);
        return make_token(keyword_type(text), text, line, col);
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
        return make_token(TOK_NUMBER, num, line, col);
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
        return make_token(TOK_STRING, text, line, col);
    }

    if (c == '-' && lexer_peek(lexer) == '>') {
        lexer_advance(lexer);
        return make_token(TOK_ARROW, strdup_cmm("->"), line, col);
    }

    switch (c) {
        case ';': return make_token(TOK_SEMICOLON, strdup_cmm(";"), line, col);
        case '=': return make_token(TOK_ASSIGN, strdup_cmm("="), line, col);
        case '(': return make_token(TOK_LPAREN, strdup_cmm("("), line, col);
        case ')': return make_token(TOK_RPAREN, strdup_cmm(")"), line, col);
        case '+': return make_token(TOK_PLUS, strdup_cmm("+"), line, col);
        case '-': return make_token(TOK_MINUS, strdup_cmm("-"), line, col);
        case '*': return make_token(TOK_STAR, strdup_cmm("*"), line, col);
        case '/': return make_token(TOK_SLASH, strdup_cmm("/"), line, col);
        case '{': return make_token(TOK_LBRACE, strdup_cmm("{"), line, col);
        case '}': return make_token(TOK_RBRACE, strdup_cmm("}"), line, col);
        case ',': return make_token(TOK_COMMA, strdup_cmm(","), line, col);
        case ':': return make_token(TOK_COLON, strdup_cmm(":"), line, col);
        default: fatal_at(line, col, "caracter no valido");
    }

    return make_token(TOK_EOF, strdup_cmm(""), line, col);
}

static void token_free(Token *t) {
    free(t->lexeme);
    t->lexeme = NULL;
}

static void parser_init(Parser *parser, const char *source) {
    parser->lexer.src = source;
    parser->lexer.pos = 0;
    parser->lexer.line = 1;
    parser->lexer.col = 1;
    parser->lexer.current = lexer_next_token(&parser->lexer);
}

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

static void append_node(ASTNode ***items, size_t *count, size_t *capacity, ASTNode *node) {
    if (*count == *capacity) {
        size_t new_capacity = (*capacity == 0) ? 8 : (*capacity * 2);
        ASTNode **new_items = (ASTNode **)realloc(*items, new_capacity * sizeof(ASTNode *));
        if (!new_items) {
            fprintf(stderr, "Error: memoria insuficiente.\n");
            exit(1);
        }
        *items = new_items;
        *capacity = new_capacity;
    }
    (*items)[(*count)++] = node;
}

static void parameter_list_add(ParameterList *list, const char *name, DataType type) {
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        char **new_names = (char **)realloc(list->names, new_capacity * sizeof(char *));
        DataType *new_types = (DataType *)realloc(list->types, new_capacity * sizeof(DataType));
        if (!new_names || !new_types) {
            free(new_names);
            free(new_types);
            fprintf(stderr, "Error: memoria insuficiente.\n");
            exit(1);
        }
        list->names = new_names;
        list->types = new_types;
        list->capacity = new_capacity;
    }
    list->names[list->count] = strdup_cmm(name);
    list->types[list->count] = type;
    list->count++;
}

static void argument_list_add(ArgumentList *list, ASTNode *arg) {
    if (list->count == list->capacity) {
        size_t new_capacity = list->capacity == 0 ? 4 : list->capacity * 2;
        ASTNode **new_items = (ASTNode **)realloc(list->items, new_capacity * sizeof(ASTNode *));
        if (!new_items) {
            fprintf(stderr, "Error: memoria insuficiente.\n");
            exit(1);
        }
        list->items = new_items;
        list->capacity = new_capacity;
    }
    list->items[list->count++] = arg;
}

static DataType parse_data_type(Parser *parser) {
    Token t = parser->lexer.current;
    DataType type;

    if (t.type == TOK_KW_INT) {
        type = TYPE_INT;
    } else if (t.type == TOK_KW_FLOAT) {
        type = TYPE_FLOAT;
    } else if (t.type == TOK_KW_STRING) {
        type = TYPE_STRING;
    } else if (t.type == TOK_KW_BOOL) {
        type = TYPE_BOOL;
    } else {
        fatal_at(t.line, t.col, "se esperaba tipo de dato");
        type = TYPE_VOID;
    }

    parser_advance(parser);
    return type;
}

static ASTNode *parse_expression(Parser *parser);

static ASTNode *parse_factor(Parser *parser) {
    Token t = parser->lexer.current;

    if (t.type == TOK_NUMBER) {
        char *lex = strdup_cmm(t.lexeme);
        parser_advance(parser);
        RuntimeValue v;
        if (strchr(lex, '.')) {
            v = runtime_make_float(strtod(lex, NULL));
        } else {
            v = runtime_make_int((int)strtol(lex, NULL, 10));
        }
        ASTNode *node = ast_create_literal(v, t.line, t.col);
        runtime_free(&v);
        free(lex);
        return node;
    }

    if (t.type == TOK_STRING) {
        RuntimeValue v = runtime_make_string(t.lexeme);
        parser_advance(parser);
        ASTNode *node = ast_create_literal(v, t.line, t.col);
        runtime_free(&v);
        return node;
    }

    if (t.type == TOK_KW_TRUE) {
        RuntimeValue v = runtime_make_bool(1);
        parser_advance(parser);
        ASTNode *node = ast_create_literal(v, t.line, t.col);
        runtime_free(&v);
        return node;
    }

    if (t.type == TOK_KW_FALSE) {
        RuntimeValue v = runtime_make_bool(0);
        parser_advance(parser);
        ASTNode *node = ast_create_literal(v, t.line, t.col);
        runtime_free(&v);
        return node;
    }

    if (t.type == TOK_IDENTIFIER) {
        char *name = strdup_cmm(t.lexeme);
        parser_advance(parser);

        if (parser_match(parser, TOK_LPAREN)) {
            ArgumentList args;
            args.items = NULL;
            args.count = 0;
            args.capacity = 0;

            if (parser->lexer.current.type != TOK_RPAREN) {
                do {
                    argument_list_add(&args, parse_expression(parser));
                } while (parser_match(parser, TOK_COMMA));
            }
            parser_expect(parser, TOK_RPAREN, "se esperaba ')' en llamada de funcion");

            ASTNode *call = ast_create_func_call(name, args.items, args.count, t.line, t.col);
            free(name);
            return call;
        }

        ASTNode *id = ast_create_identifier(name, t.line, t.col);
        free(name);
        return id;
    }

    if (parser_match(parser, TOK_LPAREN)) {
        ASTNode *expr = parse_expression(parser);
        parser_expect(parser, TOK_RPAREN, "se esperaba ')' ");
        return expr;
    }

    if (parser_match(parser, TOK_MINUS)) {
        RuntimeValue zero = runtime_make_int(0);
        ASTNode *left = ast_create_literal(zero, t.line, t.col);
        runtime_free(&zero);
        ASTNode *right = parse_factor(parser);
        return ast_create_binary_op('-', left, right, t.line, t.col);
    }

    fatal_at(t.line, t.col, "expresion invalida");
    return NULL;
}

static ASTNode *parse_term(Parser *parser) {
    ASTNode *left = parse_factor(parser);

    while (parser->lexer.current.type == TOK_STAR || parser->lexer.current.type == TOK_SLASH) {
        Token op_token = parser->lexer.current;
        parser_advance(parser);
        ASTNode *right = parse_factor(parser);
        char op = (op_token.type == TOK_STAR) ? '*' : '/';
        left = ast_create_binary_op(op, left, right, op_token.line, op_token.col);
    }

    return left;
}

static ASTNode *parse_expression(Parser *parser) {
    ASTNode *left = parse_term(parser);

    while (parser->lexer.current.type == TOK_PLUS || parser->lexer.current.type == TOK_MINUS) {
        Token op_token = parser->lexer.current;
        parser_advance(parser);
        ASTNode *right = parse_term(parser);
        char op = (op_token.type == TOK_PLUS) ? '+' : '-';
        left = ast_create_binary_op(op, left, right, op_token.line, op_token.col);
    }

    return left;
}

static ASTNode *parse_statement(Parser *parser);

static ASTNode *parse_block(Parser *parser) {
    int line = parser->lexer.current.line;
    int col = parser->lexer.current.col;
    parser_expect(parser, TOK_LBRACE, "se esperaba '{'");

    ASTNode **statements = NULL;
    size_t count = 0;
    size_t capacity = 0;

    while (parser->lexer.current.type != TOK_RBRACE && parser->lexer.current.type != TOK_EOF) {
        append_node(&statements, &count, &capacity, parse_statement(parser));
    }

    if (parser->lexer.current.type == TOK_EOF) {
        fatal_at(parser->lexer.current.line, parser->lexer.current.col, "se esperaba '}' para cerrar bloque");
    }

    parser_expect(parser, TOK_RBRACE, "se esperaba '}'");
    return ast_create_block(statements, count, line, col);
}

static ASTNode *parse_declaration(Parser *parser) {
    Token type_token = parser->lexer.current;
    DataType declared_type = parse_data_type(parser);

    if (parser->lexer.current.type != TOK_IDENTIFIER) {
        fatal_at(parser->lexer.current.line, parser->lexer.current.col, "se esperaba identificador");
    }

    Token name_token = parser->lexer.current;
    char *name = strdup_cmm(name_token.lexeme);
    parser_advance(parser);

    ASTNode *value = NULL;
    if (parser_match(parser, TOK_ASSIGN)) {
        value = parse_expression(parser);
    }

    parser_expect(parser, TOK_SEMICOLON, "se esperaba ';' al final de declaracion");
    ASTNode *decl = ast_create_var_decl(name, declared_type, value, type_token.line, type_token.col);
    free(name);
    return decl;
}

static ASTNode *parse_assignment(Parser *parser) {
    Token name_token = parser->lexer.current;
    char *name = strdup_cmm(name_token.lexeme);
    parser_advance(parser);

    parser_expect(parser, TOK_ASSIGN, "se esperaba '=' en asignacion");
    ASTNode *value = parse_expression(parser);
    parser_expect(parser, TOK_SEMICOLON, "se esperaba ';' al final de asignacion");

    ASTNode *assign = ast_create_assign(name, value, name_token.line, name_token.col);
    free(name);
    return assign;
}

static ASTNode *parse_print(Parser *parser) {
    Token t = parser->lexer.current;
    parser_expect(parser, TOK_KW_PRINT, "se esperaba 'print'");
    parser_expect(parser, TOK_LPAREN, "se esperaba '(' despues de print");
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOK_RPAREN, "se esperaba ')' ");
    parser_expect(parser, TOK_SEMICOLON, "se esperaba ';' al final de print");
    return ast_create_print(expr, t.line, t.col);
}

static ASTNode *parse_return(Parser *parser) {
    Token t = parser->lexer.current;
    parser_expect(parser, TOK_KW_RETURN, "se esperaba 'return'");

    ASTNode *expr = NULL;
    if (parser->lexer.current.type != TOK_SEMICOLON) {
        expr = parse_expression(parser);
    }

    parser_expect(parser, TOK_SEMICOLON, "se esperaba ';' al final de return");
    return ast_create_return(expr, t.line, t.col);
}

static ASTNode *parse_func_decl(Parser *parser) {
    Token func_token = parser->lexer.current;
    parser_expect(parser, TOK_KW_FUNC, "se esperaba 'func'");

    if (parser->lexer.current.type != TOK_IDENTIFIER) {
        fatal_at(parser->lexer.current.line, parser->lexer.current.col, "se esperaba nombre de funcion");
    }

    char *name = strdup_cmm(parser->lexer.current.lexeme);
    parser_advance(parser);

    parser_expect(parser, TOK_LPAREN, "se esperaba '(' en declaracion de funcion");

    ParameterList params;
    params.names = NULL;
    params.types = NULL;
    params.count = 0;
    params.capacity = 0;

    if (parser->lexer.current.type != TOK_RPAREN) {
        do {
            if (parser->lexer.current.type != TOK_IDENTIFIER) {
                fatal_at(parser->lexer.current.line, parser->lexer.current.col, "se esperaba nombre de parametro");
            }
            char *param_name = strdup_cmm(parser->lexer.current.lexeme);
            parser_advance(parser);
            parser_expect(parser, TOK_COLON, "se esperaba ':' en parametro");
            DataType param_type = parse_data_type(parser);
            parameter_list_add(&params, param_name, param_type);
            free(param_name);
        } while (parser_match(parser, TOK_COMMA));
    }

    parser_expect(parser, TOK_RPAREN, "se esperaba ')' en declaracion de funcion");
    parser_expect(parser, TOK_ARROW, "se esperaba '->' en declaracion de funcion");
    DataType return_type = parse_data_type(parser);

    ASTNode *body = parse_block(parser);
    ASTNode *node = ast_create_func_decl(
        name,
        return_type,
        params.names,
        params.types,
        params.count,
        body,
        func_token.line,
        func_token.col);

    free(name);
    return node;
}

static ASTNode *parse_statement(Parser *parser) {
    TokenType t = parser->lexer.current.type;

    if (t == TOK_LBRACE) {
        return parse_block(parser);
    }

    if (t == TOK_KW_INT || t == TOK_KW_FLOAT || t == TOK_KW_STRING || t == TOK_KW_BOOL) {
        return parse_declaration(parser);
    }

    if (t == TOK_IDENTIFIER) {
        return parse_assignment(parser);
    }

    if (t == TOK_KW_PRINT) {
        return parse_print(parser);
    }

    if (t == TOK_KW_FUNC) {
        return parse_func_decl(parser);
    }

    if (t == TOK_KW_RETURN) {
        return parse_return(parser);
    }

    fatal_at(parser->lexer.current.line, parser->lexer.current.col, "sentencia no reconocida");
    return NULL;
}

static ASTNode *parse_program(Parser *parser) {
    ASTNode **statements = NULL;
    size_t count = 0;
    size_t capacity = 0;

    while (parser->lexer.current.type != TOK_EOF) {
        append_node(&statements, &count, &capacity, parse_statement(parser));
    }

    return ast_create_program(statements, count, 1, 1);
}

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

static void run_source(const char *source) {
    Parser parser;
    parser_init(&parser, source);

    ASTNode *program = parse_program(&parser);
    token_free(&parser.lexer.current);

    Interpreter interpreter;
    interpreter_init(&interpreter, program);
    interpreter_execute(&interpreter);
    interpreter_free(&interpreter);

    ast_free(program);
}

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
