//
// Created by nikola-miljkovic on 5/19/16.
//

#ifndef NIKI_ASM_SYMBOL_H
#define NIKI_ASM_SYMBOL_H


#include <stdint.h>
#include <parser/parser.h>

enum symbol_section {
    SYMBOL_SECTION_TEXT,
    SYMBOL_SECTION_DATA,
    SYMBOL_SECTION_BSS,
    SYMBOL_SECTION_NONE,
    SYMBOL_SECTION_END,
};

enum symbol_scope {
    SYMBOL_SCOPE_GLOBAL,
    SYMBOL_SCOPE_LOCAL,
};

enum symbol_type {
    SYMBOL_TYPE_SECTION,
    SYMBOL_TYPE_EXTERN,
    SYMBOL_TYPE_DATA,
    SYMBOL_TYPE_FUNCTION,
};

struct symdata_t {
    uint32_t    index;
    char        name[MAX_LABEL_SIZE];
    uint8_t     section;
    uint8_t     scope;
    uint8_t     type;
    uint32_t    offset;
    uint32_t    size;
};

struct symtable_node_t {
    struct symdata_t* value;
    struct symtable_node_t* next;
};

struct symtable_t {
    uint32_t length;
    struct symtable_node_t* head;
};

/* creates empty symtable */
struct symtable_t* symtable_create();

/* adds symbol to symtable returns -1 if error occurred(symbol exists) */
int symtable_add_symbol(
        struct symtable_t *t,
        char *name,
        uint8_t section,
        uint8_t scope,
        uint8_t type,
        uint32_t offset,
        uint32_t size
);

struct symdata_t* symtable_get_symdata(struct symtable_t* symtable, uint32_t index);
struct symdata_t* symtable_get_symdata_by_name(struct symtable_t* symtable, const char* name);
void symtable_destroy(struct symtable_t **symtable_ptr);
void symtable_dump_to_buffer(struct symtable_t* symtable, uint8_t* buffer);

#endif //NIKI_ASM_SYMBOL_H