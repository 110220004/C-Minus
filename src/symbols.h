#ifndef SYMBOLS_H
#define SYMBOLS_H

#include <stddef.h>

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_STRING,
    TYPE_BOOL,
    TYPE_VOID
} DataType;

typedef struct {
    DataType type;
    union {
        int as_int;
        double as_float;
        char *as_string;
        int as_bool;
    } data;
} RuntimeValue;

typedef struct {
    char *name;
    DataType type;
    RuntimeValue value;
    int initialized;
} SymbolEntry;

RuntimeValue runtime_make_int(int x);
RuntimeValue runtime_make_float(double x);
RuntimeValue runtime_make_string(const char *s);
RuntimeValue runtime_make_bool(int x);
RuntimeValue runtime_make_void(void);

RuntimeValue runtime_copy(RuntimeValue v);
void runtime_free(RuntimeValue *v);
int runtime_is_numeric(DataType type);
const char *runtime_type_name(DataType type);
int runtime_convert(RuntimeValue input, DataType target_type, RuntimeValue *out);

#endif
