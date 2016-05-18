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

#define EXTRA_FUNCTION_PRE_INC "preinc"
#define EXTRA_FUNCTION_POST_INC "postinc"
#define EXTRA_FUNCTION_PRE_DEC "predec"
#define EXTRA_FUNCTION_POST_DEC "postdec"

#define AS_REGISTER_PC_STR "pc"
#define AS_REGISTER_LR_STR "lr"
#define AS_REGISTER_SP_STR "sp"
#define AS_REGISTER_PSW_STR "psw"

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
    { SECTION_NAME_END,     0       },
};

const static int DIRECTIVE_COUNT = sizeof(directive_info) / sizeof(directive_info_t);

enum argument_type {
    ARGUMENT_TYPE_IMMEDIATE,
    ARGUMENT_TYPE_REGISTER,
    ARGUMENT_TYPE_EXTRA,
    ARGUMENT_TYPE_SYMBOL,
    ARGUMENT_TYPE_NONE,
    ARGUMENT_TYPE_ERROR,
    ARGUMENT_TYPE_END,
};

typedef struct {
    int32_t     value;
    uint32_t    type; // value -1 means its variable based on arguments
} argument_info_t;

typedef struct {
    char        name[MAX_LABEL_SIZE];
    uint32_t    opcode:31;
    uint32_t    cf:1;
} instruction_info_t;

const static instruction_info_t instruction_info[] = {
    /* name     opcode      cf */
    { "int",    OP_INT,     0 },
    { "add",    OP_ADD,     0 },
    { "sub",    OP_SUB,     0 },
    { "mul",    OP_MUL,     0 },
    { "div",    OP_DIV,     0 },
    { "cmp",    OP_CMP,     0 },
    { "and",    OP_AND,     0 },
    { "or",     OP_OR,      0 },
    { "not",    OP_NOT,     0 },
    { "test",   OP_TEST,    0 },
    { "ldr",    OP_LDR,     0 },
    { "str",    OP_STR,     0 },
    { "call",   OP_CALL,    0 },
    { "in",     OP_IN,      0 },
    { "out",    OP_OUT,     0 },
    { "mov",    OP_MOV,     0 },
    { "shr",    OP_SHR,     0 },
    { "shl",    OP_SHL,     0 },
    { "ldch",   OP_LDCH,    0 },
    { "ldcl",   OP_LDCL,    0 },
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
void read_operation(union inst_t* instruction_ptr, const char* name_str);
argument_info_t read_argument(char* arg_str);
argument_info_t check_register(char* arg_str);
argument_info_t check_extra(char* arg_str);

#endif //NIKI_ASM_AS_H
