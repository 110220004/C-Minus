#ifndef AST_H
#define AST_H

#include <stddef.h>

#include "symbols.h"

typedef enum {
    AST_PROGRAM,
    AST_BLOCK,
    AST_VAR_DECL,
    AST_ASSIGN,
    AST_PRINT,
    AST_LITERAL,
    AST_IDENTIFIER,
    AST_BINARY_OP,
    AST_FUNC_DECL,
    AST_FUNC_CALL,
    AST_RETURN
} ASTNodeType;

typedef struct ASTNode ASTNode;

struct ASTNode {
    ASTNodeType type;
    int line;
    int col;
    union {
        struct {
            ASTNode **statements;
            size_t count;
        } program;
        struct {
            ASTNode **statements;
            size_t count;
        } block;
        struct {
            char *name;
            DataType declared_type;
            ASTNode *value;
        } var_decl;
        struct {
            char *name;
            ASTNode *value;
        } assign;
        struct {
            ASTNode *expression;
        } print_stmt;
        struct {
            RuntimeValue value;
        } literal;
        struct {
            char *name;
        } identifier;
        struct {
            char op;
            ASTNode *left;
            ASTNode *right;
        } binary_op;
        struct {
            char *name;
            DataType return_type;
            char **param_names;
            DataType *param_types;
            size_t param_count;
            ASTNode *body;
        } func_decl;
        struct {
            char *name;
            ASTNode **args;
            size_t arg_count;
        } func_call;
        struct {
            ASTNode *value;
        } return_stmt;
    } as;
};

ASTNode *ast_create_program(ASTNode **statements, size_t count, int line, int col);
ASTNode *ast_create_block(ASTNode **statements, size_t count, int line, int col);
ASTNode *ast_create_var_decl(const char *name, DataType declared_type, ASTNode *value, int line, int col);
ASTNode *ast_create_assign(const char *name, ASTNode *value, int line, int col);
ASTNode *ast_create_print(ASTNode *expression, int line, int col);
ASTNode *ast_create_literal(RuntimeValue value, int line, int col);
ASTNode *ast_create_identifier(const char *name, int line, int col);
ASTNode *ast_create_binary_op(char op, ASTNode *left, ASTNode *right, int line, int col);
ASTNode *ast_create_func_decl(
    const char *name,
    DataType return_type,
    char **param_names,
    DataType *param_types,
    size_t param_count,
    ASTNode *body,
    int line,
    int col);
ASTNode *ast_create_func_call(const char *name, ASTNode **args, size_t arg_count, int line, int col);
ASTNode *ast_create_return(ASTNode *value, int line, int col);

void ast_free(ASTNode *node);

#endif
