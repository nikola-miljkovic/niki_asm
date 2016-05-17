#ifndef NIKI_ASM_AS_H
#define NIKI_ASM_AS_H

#include <stdint.h>
#include <parser/parser.h>
#include "instruction.h"

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
#define SECTION_NAME_END    "end"

#define DIRECTIVE_NAME_ALIGN    "align"
#define DIRECTIVE_NAME_SKIP     "skip"

typedef struct {
    char        directive[MAX_LABEL_SIZE];
    int32_t     size; // value -1 means its variable based on arguments
} directive_info_t;

const static directive_info_t directive_info[] = {
        { "public",             0       },
        { "global",             0       },
        { "extern",             0       },
        { "char",               1       },
        { "word",               2       },
        { "long",               4       },
        { DIRECTIVE_NAME_ALIGN, -1      },
        { DIRECTIVE_NAME_SKIP,  -1      },
        { SECTION_NAME_TEXT,    0       },
        { SECTION_NAME_DATA,    0       },
        { SECTION_NAME_BSS,     0       },
        { SECTION_NAME_END,     0       }

};
const static int DIRECTIVE_COUNT = sizeof(directive_info) / sizeof(directive_info_t);

typedef struct {
    char        instruction[MAX_LABEL_SIZE];
    uint32_t    opcode; // value -1 means its variable based on arguments
} instruction_info_t;

const static instruction_info_t instruction_info[] = {
        { "int", OP_INT },
        { "add", OP_ADD },
        { "sub", OP_SUB },
        { "mul", OP_MUL },
        { "div", OP_DIV },
        { "cmp", OP_CMP },
        { "and", OP_AND },
        { "or", OP_OR },
        { "not", OP_NOT },
        { "test", OP_TEST },
        { "ldr", OP_LDR },
        { "str", OP_STR },
        { "call", OP_CALL },
        { "in", OP_IN },
        { "out", OP_OUT },
        { "mov", OP_MOV },
        { "shr", OP_SHR },
        { "shl", OP_SHL },
        { "ldch", OP_LDCH },
        { "ldcl", OP_LDCL },
};

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
    SYMBOL_TYPE_NOTYPE,
    SYMBOL_TYPE_SECTION,
    SYMBOL_TYPE_OBJECT,
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
struct symdata_t* symtable_get_symdata_by_name(struct symtable_t* symtable, char* name);
void symtable_destroy(struct symtable_t **symtable_ptr);
int32_t get_directive_size(const line_content_t* line_content);
union inst_t get_instruction(const line_content_t* line_content);
int32_t get_register(char* arg);

#endif //NIKI_ASM_AS_H
