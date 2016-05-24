#include <stdlib.h>
#include <string.h>
#include <parser/parser.h>
#include <ctype.h>
#include <parser/string_util.h>

#include "as.h"
#include "instruction.h"
#include "symbol.h"
#include "reloc.h"

static const char* condition_info[] = {
        "eq",
        "ne",
        "gt",
        "ge",
        "lt",
        "le",
        "unused",
        "",
};

int32_t get_directive_size(const line_content_t* line_content) {
    // directive index
    int i = 0;
    for (; i < DIRECTIVE_COUNT; i++) {
        if (strcmp(line_content->name, directive_info[i].directive) == 0)
            break;
    }

    // check if we got invalid directive
    if (i == DIRECTIVE_COUNT)
        return -1;
    else if (directive_info[i].size != -1)
        return directive_info[i].size;

    // in case of variable size of directive
    if (strcmp(directive_info[i].directive,"skip") == 0) {
        return atoi(line_content[i].args[0]);
    }
    // TODO: add align
    return 0;
}


union instruction
get_instruction(struct elf_context* context, const line_content_t *line_content) {
    union instruction instruction;
    struct sym_entry* symdata = NULL;
    argument_info_t arg[MAX_INSTRUCTON_ARGUMENTS];

    read_operation(&instruction, line_content->name);


    // TODO: Error handling and error handling for special registers
    // TODO: Handle symbol handling
    switch (instruction.instruction.opcode) {
        case OP_INT:
            READ_ARGS(1);
            if (arg[0].type == ARGUMENT_TYPE_IMMEDIATE) {
                instruction.int_op.src = arg[0].type;
            } else if (arg[0].type == ARGUMENT_TYPE_SYMBOL) {
                symdata = symtable_get_symdata_by_name(context->symtable, line_content->args[0]);
                if (symdata == NULL) {
                    // TODO: Error
                } else {
                    reloc_table_add(context->reloctable, symdata->index, context->location_counter);
                    instruction.int_op.src = 0;
                }
            }
            break;
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
            READ_ARGS(2);
            if (arg[0].type != ARGUMENT_TYPE_REGISTER || arg[0].type == ARGUMENT_TYPE_ERROR) {
                // error
            } else if (arg[1].type == ARGUMENT_TYPE_IMMEDIATE) {
                instruction.arithmetic_op_imm.address_type_bit = 1;
                instruction.arithmetic_op_imm.dst = (uint32_t)arg[0].value;
                instruction.arithmetic_op_imm.imm = (uint32_t)arg[1].value;
            } else if (arg[1].type == ARGUMENT_TYPE_REGISTER) {
                instruction.arithmetic_op_src.address_type_bit = 0;
                instruction.arithmetic_op_src.dst = (uint32_t)arg[0].value;
                instruction.arithmetic_op_src.src = (uint32_t)arg[1].value;
            } else if (arg[1].type == ARGUMENT_TYPE_SYMBOL) {
                instruction.arithmetic_op_imm.address_type_bit = 1;
                instruction.arithmetic_op_imm.dst = (uint32_t)arg[0].value;
                symdata = symtable_get_symdata_by_name(context->symtable, line_content->args[1]);
                if (symdata == NULL) {
                    // TODO: Error
                } else {
                    reloc_table_add(context->reloctable, symdata->index, context->location_counter);
                    instruction.arithmetic_op_imm.imm = 0;
                }
            }
            break;

        case OP_CMP:
        case OP_AND:
        case OP_OR:
        case OP_NOT:
        case OP_TEST:
            READ_ARGS(2);
            if (arg[0].type == ARGUMENT_TYPE_REGISTER && arg[1].type == ARGUMENT_TYPE_REGISTER) {
                instruction.logical_op.dst = (uint32_t)arg[0].value;
                instruction.logical_op.src = (uint32_t)arg[1].value;
            } else {
                // TODO: Handle errors
            }
            break;

        case OP_LDR:
        //case OP_STR:
            READ_ARGS(3);
            if (arg[0].type != ARGUMENT_TYPE_REGISTER ||
                (arg[1].type != ARGUMENT_TYPE_IMMEDIATE && arg[1].type != ARGUMENT_TYPE_SYMBOL)) {
                // TODO: Handle errors
            } else {

            }
            break;

        case OP_CALL:
            READ_ARGS(2);
            if (arg[0].type != ARGUMENT_TYPE_REGISTER ||
                (arg[1].type != ARGUMENT_TYPE_IMMEDIATE && arg[1].type != ARGUMENT_TYPE_SYMBOL)) {
                // TODO: handle errors
            } else {

                if (arg[1].type == ARGUMENT_TYPE_SYMBOL) {
                    instruction.call_op.dst = (uint32_t)arg[0].value;
                    symdata = symtable_get_symdata_by_name(context->symtable, line_content->args[1]);
                    if (symdata == NULL) {
                        // TODO: Error
                    } else {
                        reloc_table_add(context->reloctable, symdata->index, context->location_counter);
                        instruction.arithmetic_op_imm.imm = 0;
                    }
                }
            }

            break;

        case OP_IN:
        //case OP_OUT:
            break;

        case OP_MOV:
        //case OP_SHR:
        //case OP_SHL:
            break;

        default:
            break;
    }
    return instruction;
}

void read_operation(union instruction* instruction_ptr, const char *name_str) {
    for (uint32_t i = 0; i < OPCODES_END; i += 1) {
        for (uint32_t operation = 0; operation < OPERATION_CONDITION_END; operation += 1) {
            if (strutil_consists_of(name_str, instruction_info[i].name, condition_info[operation])) {
                instruction_ptr->instruction.opcode = instruction_info[i].opcode;
                instruction_ptr->instruction.cf = instruction_info[i].cf;
                instruction_ptr->instruction.condition = operation;
                return;
            }
        }
    }
}

argument_info_t
read_argument(const char* arg_str) {
    int i = 0;
    size_t arglen = strlen(arg_str);
    if (arglen == 0) {
        return (argument_info_t){ 0, ARGUMENT_TYPE_NONE };
    }
    // check if it is valid register string
    // if so return reg_number else return -1
    // format is R|r<0-15>\0
    //
    if ((arg_str[0] == 'r' || arg_str[0] == 'R') && strlen(arg_str) > 1) {
        for (i = 1; i < arglen; i += 1) {
            if (!isdigit(arg_str[i])) {
                goto ret_error;
            }
        }
        int32_t reg_num = atoi(arg_str + 1);
        if (reg_num > 15 || reg_num < 0)
            goto ret_error;
        return (argument_info_t){ reg_num, ARGUMENT_TYPE_REGISTER };
    }
    // check if argument is immediate value, +|-(num)|(num)
    //
    else if (((arg_str[0] == '+' || arg_str[0] == '-') && arglen > 1)
            || isdigit(arg_str[0])){
        for (i = 1; i < strlen(arg_str); i += 1) {
            if (!isdigit(arg_str[i])) {
                goto ret_error;
            }
        }
        int32_t imm_val = atoi(arg_str);
        return (argument_info_t){ imm_val, ARGUMENT_TYPE_IMMEDIATE };
    }
    // check if argument is special register, symbolname or extra func
    //
    else {
        argument_info_t reg = check_register(arg_str);
        if (reg.type == ARGUMENT_TYPE_REGISTER)
            return reg;

        // check if argument is postinc preinc postdec predec
        //
        argument_info_t extra = check_extra(arg_str);
        if (extra.type == ARGUMENT_TYPE_EXTRA)
            return extra;

        // check if argument is symbolname
        //
        for (i = 0; i < arglen; i += 1) {
            if (!isalnum(arg_str[i])) {
                goto ret_error;
            }
        }
        // return type as symbol, and let caller handle it
        return (argument_info_t){ 0, ARGUMENT_TYPE_SYMBOL };
    }
ret_error:
    return (argument_info_t){ 0, ARGUMENT_TYPE_ERROR };
}

argument_info_t
check_extra(const char* arg_str) {
    if (strutil_is_equal(arg_str, EXTRA_FUNCTION_POST_INC))
        return (argument_info_t){ EXTRA_REG_FUNCTION_POST_INC, ARGUMENT_TYPE_EXTRA };
    if (strutil_is_equal(arg_str, EXTRA_FUNCTION_PRE_INC))
        return (argument_info_t){ EXTRA_REG_FUNCTION_PRE_INC, ARGUMENT_TYPE_EXTRA };
    if (strutil_is_equal(arg_str, EXTRA_FUNCTION_PRE_DEC))
        return (argument_info_t){ EXTRA_REG_FUNCTION_PRE_DEC, ARGUMENT_TYPE_EXTRA };
    if (strutil_is_equal(arg_str, EXTRA_FUNCTION_POST_DEC))
        return (argument_info_t){ EXTRA_REG_FUNCTION_POST_DEC, ARGUMENT_TYPE_EXTRA };
    return (argument_info_t){ 0, ARGUMENT_TYPE_ERROR };
}

argument_info_t
check_register(const char* arg_str) {
    if (strutil_is_equal(arg_str, AS_REGISTER_PC_STR))
        return (argument_info_t){ AS_REGISTER_PC, ARGUMENT_TYPE_REGISTER };
    if (strutil_is_equal(arg_str, AS_REGISTER_SP_STR))
        return (argument_info_t){ AS_REGISTER_SP, ARGUMENT_TYPE_REGISTER };
    if (strutil_is_equal(arg_str, AS_REGISTER_LR_STR))
        return (argument_info_t){ AS_REGISTER_LR, ARGUMENT_TYPE_REGISTER };
    if (strutil_is_equal(arg_str, AS_REGISTER_PSW_STR))
        return (argument_info_t){ AS_REGISTER_PSW, ARGUMENT_TYPE_REGISTER };
    return (argument_info_t){ 0, ARGUMENT_TYPE_ERROR };
}