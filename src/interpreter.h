#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stddef.h>

#include "ast.h"
#include "scope.h"
#include "symbols.h"

typedef struct {
    RuntimeValue value;
    int is_return;
} EvalResult;

typedef struct {
    char *name;
    ASTNode *decl;
} FunctionDef;

typedef struct {
    ASTNode *program;
    ScopeManager *scope_manager;
    FunctionDef *functions;
    size_t function_count;
    size_t function_capacity;
    int call_depth;
} Interpreter;

void interpreter_init(Interpreter *interpreter, ASTNode *program);
void interpreter_free(Interpreter *interpreter);
void interpreter_execute(Interpreter *interpreter);

#endif
