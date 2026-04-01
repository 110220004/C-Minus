#ifndef SCOPE_H
#define SCOPE_H

#include <stddef.h>

#include "symbols.h"

typedef struct Scope {
    SymbolEntry *symbols;
    size_t count;
    size_t capacity;
    struct Scope *parent;
    char *scope_name;
} Scope;

typedef struct {
    Scope *global;
    Scope *current;
} ScopeManager;

ScopeManager *scope_manager_create(void);
void scope_manager_destroy(ScopeManager *manager);

int scope_manager_push(ScopeManager *manager, const char *name);
int scope_manager_pop(ScopeManager *manager);

SymbolEntry *scope_manager_define(ScopeManager *manager, const char *name, DataType type);
SymbolEntry *scope_manager_lookup(ScopeManager *manager, const char *name);
SymbolEntry *scope_manager_lookup_local(ScopeManager *manager, const char *name);

#endif
