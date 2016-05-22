#ifndef NIKI_ASM_AS_H
#define NIKI_ASM_AS_H

#include <stdint.h>
#include <parser/parser.h>
#include "instruction.h"

#define READ_ARGS(n) for (int i = 0; i < n; i += 1) arg[i] = read_argument(line_content->args[i]);

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
#define DIRECTIVE_NAME_EXTERN   "extern"
#define DIRECTIVE_NAME_PUBLIC   "public"
#define DIRECTIVE_NAME_GLOBAL   "global"

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
    { "add",    OP_ADD,     1 },
    { "sub",    OP_SUB,     1 },
    { "mul",    OP_MUL,     1 },
    { "div",    OP_DIV,     1 },
    { "cmp",    OP_CMP,     1 },
    { "and",    OP_AND,     1 },
    { "or",     OP_OR,      1 },
    { "not",    OP_NOT,     1 },
    { "test",   OP_TEST,    1 },
    { "ldr",    OP_LDR,     0 },
    { "str",    OP_STR,     0 },
    { "call",   OP_CALL,    0 },
    { "in",     OP_IN,      0 },
    { "out",    OP_OUT,     0 },
    { "mov",    OP_MOV,     1 },
    { "shr",    OP_SHR,     1 },
    { "shl",    OP_SHL,     1 },
    { "ldch",   OP_LDCH,    0 },
    { "ldcl",   OP_LDCL,    0 },
};

struct elf_context {
    struct symtable_t*        symtable;
    struct reloc_table_t*     reloctable;
    uint32_t                  location_counter;
};

int32_t get_directive_size(const line_content_t* line_content);
union inst_t get_instruction(struct elf_context *context, const line_content_t *line_content);
void read_operation(union inst_t* instruction_ptr, const char* name_str);
argument_info_t read_argument(const char* arg_str);
argument_info_t check_register(const char* arg_str);
argument_info_t check_extra(const char* arg_str);

#endif //NIKI_ASM_AS_H
