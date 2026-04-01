#include "interpreter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void interpreter_error_node(ASTNode *node, const char *message) {
    int line = node ? node->line : 0;
    int col = node ? node->col : 0;
    fprintf(stderr, "Error (%d:%d): %s\n", line, col, message);
    exit(1);
}

static RuntimeValue runtime_default_for_type(DataType type) {
    switch (type) {
        case TYPE_INT: return runtime_make_int(0);
        case TYPE_FLOAT: return runtime_make_float(0.0);
        case TYPE_STRING: return runtime_make_string("");
        case TYPE_BOOL: return runtime_make_bool(0);
        case TYPE_VOID: return runtime_make_void();
        default: return runtime_make_void();
    }
}

static void append_function_def(Interpreter *interpreter, const char *name, ASTNode *decl) {
    if (interpreter->function_count == interpreter->function_capacity) {
        size_t new_capacity = interpreter->function_capacity == 0 ? 8 : interpreter->function_capacity * 2;
        FunctionDef *new_functions =
            (FunctionDef *)realloc(interpreter->functions, new_capacity * sizeof(FunctionDef));
        if (!new_functions) {
            fprintf(stderr, "Error: memoria insuficiente.\n");
            exit(1);
        }
        interpreter->functions = new_functions;
        interpreter->function_capacity = new_capacity;
    }

    FunctionDef *slot = &interpreter->functions[interpreter->function_count++];
    slot->name = (char *)malloc(strlen(name) + 1);
    if (!slot->name) {
        fprintf(stderr, "Error: memoria insuficiente.\n");
        exit(1);
    }
    strcpy(slot->name, name);
    slot->decl = decl;
}

static FunctionDef *find_function(Interpreter *interpreter, const char *name) {
    for (size_t i = 0; i < interpreter->function_count; i++) {
        if (strcmp(interpreter->functions[i].name, name) == 0) {
            return &interpreter->functions[i];
        }
    }
    return NULL;
}

static void register_functions(Interpreter *interpreter) {
    if (!interpreter->program || interpreter->program->type != AST_PROGRAM) {
        interpreter_error_node(interpreter->program, "AST de programa invalido");
    }

    for (size_t i = 0; i < interpreter->program->as.program.count; i++) {
        ASTNode *stmt = interpreter->program->as.program.statements[i];
        if (stmt->type != AST_FUNC_DECL) {
            continue;
        }
        if (find_function(interpreter, stmt->as.func_decl.name)) {
            interpreter_error_node(stmt, "funcion redeclarada");
        }
        append_function_def(interpreter, stmt->as.func_decl.name, stmt);
    }
}

static EvalResult eval_node(Interpreter *interpreter, ASTNode *node);

static void assign_symbol(ASTNode *node, SymbolEntry *symbol, RuntimeValue value) {
    RuntimeValue converted;
    if (!runtime_convert(value, symbol->type, &converted)) {
        runtime_free(&value);
        interpreter_error_node(node, "asignacion invalida por tipo");
    }

    runtime_free(&value);
    if (symbol->initialized) {
        runtime_free(&symbol->value);
    }
    symbol->value = converted;
    symbol->initialized = 1;
}

static char *value_to_cstring(RuntimeValue value) {
    char buffer[128];
    const char *src = NULL;

    if (value.type == TYPE_STRING) {
        src = value.data.as_string ? value.data.as_string : "";
    } else if (value.type == TYPE_INT) {
        snprintf(buffer, sizeof(buffer), "%d", value.data.as_int);
        src = buffer;
    } else if (value.type == TYPE_FLOAT) {
        snprintf(buffer, sizeof(buffer), "%g", value.data.as_float);
        src = buffer;
    } else if (value.type == TYPE_BOOL) {
        src = value.data.as_bool ? "true" : "false";
    } else {
        src = "void";
    }

    char *out = (char *)malloc(strlen(src) + 1);
    if (!out) {
        fprintf(stderr, "Error: memoria insuficiente.\n");
        exit(1);
    }
    strcpy(out, src);
    return out;
}

static EvalResult eval_program(Interpreter *interpreter, ASTNode *node) {
    EvalResult result;
    result.value = runtime_make_void();
    result.is_return = 0;

    for (size_t i = 0; i < node->as.program.count; i++) {
        EvalResult child = eval_node(interpreter, node->as.program.statements[i]);
        if (child.is_return) {
            runtime_free(&child.value);
            interpreter_error_node(node->as.program.statements[i], "return fuera de funcion");
        }
        runtime_free(&child.value);
    }

    return result;
}

static EvalResult eval_block(Interpreter *interpreter, ASTNode *node, int create_scope, const char *scope_name) {
    EvalResult result;
    result.value = runtime_make_void();
    result.is_return = 0;

    if (create_scope && !scope_manager_push(interpreter->scope_manager, scope_name)) {
        interpreter_error_node(node, "no se pudo crear scope");
    }

    for (size_t i = 0; i < node->as.block.count; i++) {
        EvalResult child = eval_node(interpreter, node->as.block.statements[i]);
        if (child.is_return) {
            if (create_scope) {
                scope_manager_pop(interpreter->scope_manager);
            }
            return child;
        }
        runtime_free(&child.value);
    }

    if (create_scope) {
        scope_manager_pop(interpreter->scope_manager);
    }

    return result;
}

static EvalResult eval_binary_op(Interpreter *interpreter, ASTNode *node) {
    (void)interpreter;
    EvalResult left_res = eval_node(interpreter, node->as.binary_op.left);
    EvalResult right_res = eval_node(interpreter, node->as.binary_op.right);

    if (left_res.is_return || right_res.is_return) {
        runtime_free(&left_res.value);
        runtime_free(&right_res.value);
        interpreter_error_node(node, "return inesperado en expresion");
    }

    RuntimeValue left = left_res.value;
    RuntimeValue right = right_res.value;
    char op = node->as.binary_op.op;

    if (op == '+' && (left.type == TYPE_STRING || right.type == TYPE_STRING)) {
        char *a = value_to_cstring(left);
        char *b = value_to_cstring(right);
        size_t len = strlen(a) + strlen(b);
        char *joined = (char *)malloc(len + 1);
        if (!joined) {
            fprintf(stderr, "Error: memoria insuficiente.\n");
            exit(1);
        }
        strcpy(joined, a);
        strcat(joined, b);

        free(a);
        free(b);
        runtime_free(&left);
        runtime_free(&right);

        EvalResult out;
        out.value.type = TYPE_STRING;
        out.value.data.as_string = joined;
        out.is_return = 0;
        return out;
    }

    if (!runtime_is_numeric(left.type) || !runtime_is_numeric(right.type)) {
        runtime_free(&left);
        runtime_free(&right);
        interpreter_error_node(node, "operacion aritmetica invalida");
    }

    int use_float = (left.type == TYPE_FLOAT || right.type == TYPE_FLOAT);
    EvalResult out;
    out.is_return = 0;

    if (use_float) {
        double a = (left.type == TYPE_FLOAT) ? left.data.as_float : (double)left.data.as_int;
        double b = (right.type == TYPE_FLOAT) ? right.data.as_float : (double)right.data.as_int;

        if (op == '/' && b == 0.0) {
            runtime_free(&left);
            runtime_free(&right);
            interpreter_error_node(node, "division por cero");
        }

        if (op == '+') out.value = runtime_make_float(a + b);
        else if (op == '-') out.value = runtime_make_float(a - b);
        else if (op == '*') out.value = runtime_make_float(a * b);
        else if (op == '/') out.value = runtime_make_float(a / b);
        else {
            runtime_free(&left);
            runtime_free(&right);
            interpreter_error_node(node, "operador binario no soportado");
        }
    } else {
        int a = left.data.as_int;
        int b = right.data.as_int;

        if (op == '/' && b == 0) {
            runtime_free(&left);
            runtime_free(&right);
            interpreter_error_node(node, "division por cero");
        }

        if (op == '+') out.value = runtime_make_int(a + b);
        else if (op == '-') out.value = runtime_make_int(a - b);
        else if (op == '*') out.value = runtime_make_int(a * b);
        else if (op == '/') out.value = runtime_make_int(a / b);
        else {
            runtime_free(&left);
            runtime_free(&right);
            interpreter_error_node(node, "operador binario no soportado");
        }
    }

    runtime_free(&left);
    runtime_free(&right);
    return out;
}

static EvalResult eval_func_call(Interpreter *interpreter, ASTNode *node) {
    FunctionDef *def = find_function(interpreter, node->as.func_call.name);
    if (!def) {
        interpreter_error_node(node, "llamada a funcion no declarada");
    }

    ASTNode *decl = def->decl;
    size_t expected = decl->as.func_decl.param_count;
    size_t given = node->as.func_call.arg_count;
    if (expected != given) {
        interpreter_error_node(node, "cantidad de argumentos invalida");
    }

    if (!scope_manager_push(interpreter->scope_manager, def->name)) {
        interpreter_error_node(node, "no se pudo crear scope de funcion");
    }

    for (size_t i = 0; i < expected; i++) {
        EvalResult arg_eval = eval_node(interpreter, node->as.func_call.args[i]);
        if (arg_eval.is_return) {
            runtime_free(&arg_eval.value);
            interpreter_error_node(node, "return inesperado en argumento");
        }

        SymbolEntry *param = scope_manager_define(
            interpreter->scope_manager,
            decl->as.func_decl.param_names[i],
            decl->as.func_decl.param_types[i]);
        if (!param) {
            runtime_free(&arg_eval.value);
            interpreter_error_node(node, "parametro redeclarado");
        }

        assign_symbol(node, param, arg_eval.value);
    }

    interpreter->call_depth++;
    EvalResult body_res = eval_block(interpreter, decl->as.func_decl.body, 0, def->name);
    interpreter->call_depth--;

    scope_manager_pop(interpreter->scope_manager);

    RuntimeValue return_value;
    if (body_res.is_return) {
        return_value = body_res.value;
    } else {
        return_value = runtime_default_for_type(decl->as.func_decl.return_type);
    }

    RuntimeValue converted;
    if (!runtime_convert(return_value, decl->as.func_decl.return_type, &converted)) {
        runtime_free(&return_value);
        interpreter_error_node(node, "tipo de retorno invalido");
    }

    runtime_free(&return_value);

    EvalResult out;
    out.value = converted;
    out.is_return = 0;
    return out;
}

static EvalResult eval_node(Interpreter *interpreter, ASTNode *node) {
    EvalResult result;
    result.value = runtime_make_void();
    result.is_return = 0;

    switch (node->type) {
        case AST_PROGRAM:
            return eval_program(interpreter, node);

        case AST_BLOCK:
            return eval_block(interpreter, node, 1, "block");

        case AST_VAR_DECL: {
            SymbolEntry *entry = scope_manager_define(
                interpreter->scope_manager,
                node->as.var_decl.name,
                node->as.var_decl.declared_type);
            if (!entry) {
                interpreter_error_node(node, "variable redeclarada en el mismo ambito");
            }
            if (node->as.var_decl.value) {
                EvalResult expr = eval_node(interpreter, node->as.var_decl.value);
                if (expr.is_return) {
                    runtime_free(&expr.value);
                    interpreter_error_node(node, "return inesperado en declaracion");
                }
                assign_symbol(node, entry, expr.value);
            }
            return result;
        }

        case AST_ASSIGN: {
            SymbolEntry *entry = scope_manager_lookup(interpreter->scope_manager, node->as.assign.name);
            if (!entry) {
                interpreter_error_node(node, "asignacion a variable no declarada");
            }
            EvalResult expr = eval_node(interpreter, node->as.assign.value);
            if (expr.is_return) {
                runtime_free(&expr.value);
                interpreter_error_node(node, "return inesperado en asignacion");
            }
            assign_symbol(node, entry, expr.value);
            return result;
        }

        case AST_PRINT: {
            EvalResult expr = eval_node(interpreter, node->as.print_stmt.expression);
            if (expr.is_return) {
                runtime_free(&expr.value);
                interpreter_error_node(node, "return inesperado en print");
            }

            if (expr.value.type == TYPE_INT) {
                printf("%d\n", expr.value.data.as_int);
            } else if (expr.value.type == TYPE_FLOAT) {
                printf("%g\n", expr.value.data.as_float);
            } else if (expr.value.type == TYPE_STRING) {
                printf("%s\n", expr.value.data.as_string ? expr.value.data.as_string : "");
            } else if (expr.value.type == TYPE_BOOL) {
                printf("%s\n", expr.value.data.as_bool ? "true" : "false");
            } else {
                printf("void\n");
            }

            runtime_free(&expr.value);
            return result;
        }

        case AST_LITERAL:
            result.value = runtime_copy(node->as.literal.value);
            return result;

        case AST_IDENTIFIER: {
            SymbolEntry *entry = scope_manager_lookup(interpreter->scope_manager, node->as.identifier.name);
            if (!entry) {
                interpreter_error_node(node, "uso de variable no declarada");
            }
            if (!entry->initialized) {
                interpreter_error_node(node, "uso de variable sin inicializar");
            }
            result.value = runtime_copy(entry->value);
            return result;
        }

        case AST_BINARY_OP:
            return eval_binary_op(interpreter, node);

        case AST_FUNC_DECL:
            return result;

        case AST_FUNC_CALL:
            return eval_func_call(interpreter, node);

        case AST_RETURN:
            if (interpreter->call_depth == 0) {
                interpreter_error_node(node, "return fuera de funcion");
            }
            if (node->as.return_stmt.value) {
                EvalResult expr = eval_node(interpreter, node->as.return_stmt.value);
                if (expr.is_return) {
                    runtime_free(&expr.value);
                    interpreter_error_node(node, "return anidado invalido");
                }
                result.value = expr.value;
            } else {
                result.value = runtime_make_void();
            }
            result.is_return = 1;
            return result;
    }

    interpreter_error_node(node, "nodo AST no soportado");
    return result;
}

void interpreter_init(Interpreter *interpreter, ASTNode *program) {
    interpreter->program = program;
    interpreter->scope_manager = scope_manager_create();
    interpreter->functions = NULL;
    interpreter->function_count = 0;
    interpreter->function_capacity = 0;
    interpreter->call_depth = 0;
}

void interpreter_free(Interpreter *interpreter) {
    if (!interpreter) {
        return;
    }

    for (size_t i = 0; i < interpreter->function_count; i++) {
        free(interpreter->functions[i].name);
    }
    free(interpreter->functions);
    scope_manager_destroy(interpreter->scope_manager);
    interpreter->scope_manager = NULL;
}

void interpreter_execute(Interpreter *interpreter) {
    register_functions(interpreter);
    EvalResult result = eval_node(interpreter, interpreter->program);
    runtime_free(&result.value);
}
