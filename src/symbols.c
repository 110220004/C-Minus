#include "symbols.h"

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

RuntimeValue runtime_make_int(int x) {
    RuntimeValue v;
    v.type = TYPE_INT;
    v.data.as_int = x;
    return v;
}

RuntimeValue runtime_make_float(double x) {
    RuntimeValue v;
    v.type = TYPE_FLOAT;
    v.data.as_float = x;
    return v;
}

RuntimeValue runtime_make_string(const char *s) {
    RuntimeValue v;
    v.type = TYPE_STRING;
    v.data.as_string = dup_cstr(s ? s : "");
    return v;
}

RuntimeValue runtime_make_bool(int x) {
    RuntimeValue v;
    v.type = TYPE_BOOL;
    v.data.as_bool = x ? 1 : 0;
    return v;
}

RuntimeValue runtime_make_void(void) {
    RuntimeValue v;
    v.type = TYPE_VOID;
    v.data.as_int = 0;
    return v;
}

RuntimeValue runtime_copy(RuntimeValue v) {
    RuntimeValue out;
    out.type = v.type;
    if (v.type == TYPE_STRING) {
        out.data.as_string = dup_cstr(v.data.as_string ? v.data.as_string : "");
    } else if (v.type == TYPE_INT) {
        out.data.as_int = v.data.as_int;
    } else if (v.type == TYPE_FLOAT) {
        out.data.as_float = v.data.as_float;
    } else if (v.type == TYPE_BOOL) {
        out.data.as_bool = v.data.as_bool;
    } else {
        out.data.as_int = 0;
    }
    return out;
}

void runtime_free(RuntimeValue *v) {
    if (!v) {
        return;
    }
    if (v->type == TYPE_STRING && v->data.as_string) {
        free(v->data.as_string);
        v->data.as_string = NULL;
    }
}

int runtime_is_numeric(DataType type) {
    return type == TYPE_INT || type == TYPE_FLOAT;
}

const char *runtime_type_name(DataType type) {
    switch (type) {
        case TYPE_INT: return "int";
        case TYPE_FLOAT: return "float";
        case TYPE_STRING: return "string";
        case TYPE_BOOL: return "bool";
        case TYPE_VOID: return "void";
        default: return "desconocido";
    }
}

int runtime_convert(RuntimeValue input, DataType target_type, RuntimeValue *out) {
    if (!out) {
        return 0;
    }

    if (input.type == target_type) {
        *out = runtime_copy(input);
        return 1;
    }

    if (target_type == TYPE_FLOAT && input.type == TYPE_INT) {
        *out = runtime_make_float((double)input.data.as_int);
        return 1;
    }

    return 0;
}
