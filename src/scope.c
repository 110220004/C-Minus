#include "scope.h"

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

static void symbol_entry_free(SymbolEntry *entry) {
    if (!entry) {
        return;
    }
    free(entry->name);
    runtime_free(&entry->value);
}

static Scope *scope_create(Scope *parent, const char *name) {
    Scope *scope = (Scope *)malloc(sizeof(Scope));
    if (!scope) {
        fprintf(stderr, "Error: memoria insuficiente.\n");
        exit(1);
    }
    scope->symbols = NULL;
    scope->count = 0;
    scope->capacity = 0;
    scope->parent = parent;
    scope->scope_name = dup_cstr(name ? name : "scope");
    return scope;
}

static void scope_destroy(Scope *scope) {
    if (!scope) {
        return;
    }

    for (size_t i = 0; i < scope->count; i++) {
        symbol_entry_free(&scope->symbols[i]);
    }
    free(scope->symbols);
    free(scope->scope_name);
    free(scope);
}

ScopeManager *scope_manager_create(void) {
    ScopeManager *manager = (ScopeManager *)malloc(sizeof(ScopeManager));
    if (!manager) {
        fprintf(stderr, "Error: memoria insuficiente.\n");
        exit(1);
    }

    manager->global = scope_create(NULL, "global");
    manager->current = manager->global;
    return manager;
}

void scope_manager_destroy(ScopeManager *manager) {
    if (!manager) {
        return;
    }

    while (manager->current && manager->current != manager->global) {
        Scope *current = manager->current;
        manager->current = current->parent;
        scope_destroy(current);
    }

    scope_destroy(manager->global);
    free(manager);
}

int scope_manager_push(ScopeManager *manager, const char *name) {
    if (!manager || !manager->current) {
        return 0;
    }

    Scope *next = scope_create(manager->current, name);
    manager->current = next;
    return 1;
}

int scope_manager_pop(ScopeManager *manager) {
    if (!manager || !manager->current || manager->current == manager->global) {
        return 0;
    }

    Scope *current = manager->current;
    manager->current = current->parent;
    scope_destroy(current);
    return 1;
}

SymbolEntry *scope_manager_lookup_local(ScopeManager *manager, const char *name) {
    if (!manager || !manager->current || !name) {
        return NULL;
    }

    Scope *scope = manager->current;
    for (size_t i = 0; i < scope->count; i++) {
        if (strcmp(scope->symbols[i].name, name) == 0) {
            return &scope->symbols[i];
        }
    }
    return NULL;
}

SymbolEntry *scope_manager_lookup(ScopeManager *manager, const char *name) {
    if (!manager || !name) {
        return NULL;
    }

    for (Scope *scope = manager->current; scope != NULL; scope = scope->parent) {
        for (size_t i = 0; i < scope->count; i++) {
            if (strcmp(scope->symbols[i].name, name) == 0) {
                return &scope->symbols[i];
            }
        }
    }

    return NULL;
}

SymbolEntry *scope_manager_define(ScopeManager *manager, const char *name, DataType type) {
    if (!manager || !manager->current || !name) {
        return NULL;
    }

    if (scope_manager_lookup_local(manager, name)) {
        return NULL;
    }

    Scope *scope = manager->current;
    if (scope->count == scope->capacity) {
        size_t new_capacity = scope->capacity == 0 ? 8 : scope->capacity * 2;
        SymbolEntry *new_symbols = (SymbolEntry *)realloc(scope->symbols, new_capacity * sizeof(SymbolEntry));
        if (!new_symbols) {
            fprintf(stderr, "Error: memoria insuficiente.\n");
            exit(1);
        }
        scope->symbols = new_symbols;
        scope->capacity = new_capacity;
    }

    SymbolEntry *entry = &scope->symbols[scope->count++];
    entry->name = dup_cstr(name);
    entry->type = type;
    entry->initialized = 0;
    entry->value = runtime_make_void();
    return entry;
}
