#include "ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *dup_cstr(const char *s) {
    size_t len = strlen(s);
    char *copy = (char *)malloc(len + 1);
    if (!copy) {
        fprintf(stderr, "Error: memoria insuficiente.\n");
        exit(1);
    }
    memcpy(copy, s, len + 1);
    return copy;
}

static ASTNode *ast_alloc(ASTNodeType type, int line, int col) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) {
        fprintf(stderr, "Error: memoria insuficiente.\n");
        exit(1);
    }
    node->type = type;
    node->line = line;
    node->col = col;
    return node;
}

ASTNode *ast_create_program(ASTNode **statements, size_t count, int line, int col) {
    ASTNode *node = ast_alloc(AST_PROGRAM, line, col);
    node->as.program.statements = statements;
    node->as.program.count = count;
    return node;
}

ASTNode *ast_create_block(ASTNode **statements, size_t count, int line, int col) {
    ASTNode *node = ast_alloc(AST_BLOCK, line, col);
    node->as.block.statements = statements;
    node->as.block.count = count;
    return node;
}

ASTNode *ast_create_var_decl(const char *name, DataType declared_type, ASTNode *value, int line, int col) {
    ASTNode *node = ast_alloc(AST_VAR_DECL, line, col);
    node->as.var_decl.name = dup_cstr(name);
    node->as.var_decl.declared_type = declared_type;
    node->as.var_decl.value = value;
    return node;
}

ASTNode *ast_create_assign(const char *name, ASTNode *value, int line, int col) {
    ASTNode *node = ast_alloc(AST_ASSIGN, line, col);
    node->as.assign.name = dup_cstr(name);
    node->as.assign.value = value;
    return node;
}

ASTNode *ast_create_print(ASTNode *expression, int line, int col) {
    ASTNode *node = ast_alloc(AST_PRINT, line, col);
    node->as.print_stmt.expression = expression;
    return node;
}

ASTNode *ast_create_literal(RuntimeValue value, int line, int col) {
    ASTNode *node = ast_alloc(AST_LITERAL, line, col);
    node->as.literal.value = runtime_copy(value);
    return node;
}

ASTNode *ast_create_identifier(const char *name, int line, int col) {
    ASTNode *node = ast_alloc(AST_IDENTIFIER, line, col);
    node->as.identifier.name = dup_cstr(name);
    return node;
}

ASTNode *ast_create_binary_op(char op, ASTNode *left, ASTNode *right, int line, int col) {
    ASTNode *node = ast_alloc(AST_BINARY_OP, line, col);
    node->as.binary_op.op = op;
    node->as.binary_op.left = left;
    node->as.binary_op.right = right;
    return node;
}

ASTNode *ast_create_func_decl(
    const char *name,
    DataType return_type,
    char **param_names,
    DataType *param_types,
    size_t param_count,
    ASTNode *body,
    int line,
    int col) {
    ASTNode *node = ast_alloc(AST_FUNC_DECL, line, col);
    node->as.func_decl.name = dup_cstr(name);
    node->as.func_decl.return_type = return_type;
    node->as.func_decl.param_names = param_names;
    node->as.func_decl.param_types = param_types;
    node->as.func_decl.param_count = param_count;
    node->as.func_decl.body = body;
    return node;
}

ASTNode *ast_create_func_call(const char *name, ASTNode **args, size_t arg_count, int line, int col) {
    ASTNode *node = ast_alloc(AST_FUNC_CALL, line, col);
    node->as.func_call.name = dup_cstr(name);
    node->as.func_call.args = args;
    node->as.func_call.arg_count = arg_count;
    return node;
}

ASTNode *ast_create_return(ASTNode *value, int line, int col) {
    ASTNode *node = ast_alloc(AST_RETURN, line, col);
    node->as.return_stmt.value = value;
    return node;
}

void ast_free(ASTNode *node) {
    if (!node) {
        return;
    }

    switch (node->type) {
        case AST_PROGRAM:
            for (size_t i = 0; i < node->as.program.count; i++) {
                ast_free(node->as.program.statements[i]);
            }
            free(node->as.program.statements);
            break;

        case AST_BLOCK:
            for (size_t i = 0; i < node->as.block.count; i++) {
                ast_free(node->as.block.statements[i]);
            }
            free(node->as.block.statements);
            break;

        case AST_VAR_DECL:
            free(node->as.var_decl.name);
            ast_free(node->as.var_decl.value);
            break;

        case AST_ASSIGN:
            free(node->as.assign.name);
            ast_free(node->as.assign.value);
            break;

        case AST_PRINT:
            ast_free(node->as.print_stmt.expression);
            break;

        case AST_LITERAL:
            runtime_free(&node->as.literal.value);
            break;

        case AST_IDENTIFIER:
            free(node->as.identifier.name);
            break;

        case AST_BINARY_OP:
            ast_free(node->as.binary_op.left);
            ast_free(node->as.binary_op.right);
            break;

        case AST_FUNC_DECL:
            free(node->as.func_decl.name);
            for (size_t i = 0; i < node->as.func_decl.param_count; i++) {
                free(node->as.func_decl.param_names[i]);
            }
            free(node->as.func_decl.param_names);
            free(node->as.func_decl.param_types);
            ast_free(node->as.func_decl.body);
            break;

        case AST_FUNC_CALL:
            free(node->as.func_call.name);
            for (size_t i = 0; i < node->as.func_call.arg_count; i++) {
                ast_free(node->as.func_call.args[i]);
            }
            free(node->as.func_call.args);
            break;

        case AST_RETURN:
            ast_free(node->as.return_stmt.value);
            break;
    }

    free(node);
}
