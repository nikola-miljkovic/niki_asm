#ifndef NIKI_ASM_AS_H
#define NIKI_ASM_AS_H

#include <stdint.h>

/*
 * Supported directives:
 *
 * Name:       offsetinc:
 * -------------------------------------
 * .public      (0)
 * .global      (0)
 * .extern      (0)
 * .char        (1)
 * .word        (2)
 * .long        (4)
 * .align       (1-3)
 * .skip        (param)
 */

#define SECTION_NAME_TEXT   "text"
#define SECTION_NAME_DATA   "data"
#define SECTION_NAME_BSS    "bss"

enum symbol_section {
    SYMBOL_SECTION_TEXT,
    SYMBOL_SECTION_DATA,
    SYMBOL_SECTION_BSS,
};

enum symbol_scope {
    SYMBOL_SCOPE_GLOBAL,
    SYMBOL_SCOPE_LOCAL,
};

enum symbol_type {
    SYMBOL_TYPE_NOTYPE,
    SYMBOL_TYPE_SECTION,
    SYMBOL_TYPE_OBJECT,
    SYMBOL_TYPE_DATA,
    SYMBOL_TYPE_FUNCTION,
};

struct symdata_t {
    uint32_t index;
    char* name;
    uint8_t section;
    uint8_t scope;
    uint8_t type;
    uint32_t offset;
    uint32_t size;
};

struct symtable_t {
    struct symdata_t* value;
    struct symtable_t* next;
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

void symtable_destroy(struct symtable_t **symtable_ptr);

#endif //NIKI_ASM_AS_H
