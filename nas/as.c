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
    if (strutil_is_equal(directive_info[i].directive, DIRECTIVE_NAME_SKIP)) {
        return atoi(line_content->args[0]);
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
        case OP_CMP:
            READ_ARGS(2);
            if (arg[0].type != ARGUMENT_TYPE_REGISTER || arg[0].type == ARGUMENT_TYPE_ERROR) {
                // error
            } else if (arg[1].type == ARGUMENT_TYPE_IMMEDIATE) {
                instruction.arithmetic_op_imm.address_type_bit = 1;
                instruction.arithmetic_op_imm.dst = arg[0].value.uval;
                instruction.arithmetic_op_imm.imm = arg[1].value.uval;
            } else if (arg[1].type == ARGUMENT_TYPE_REGISTER) {
                instruction.arithmetic_op_src.address_type_bit = 0;
                instruction.arithmetic_op_src.dst = arg[0].value.uval;
                instruction.arithmetic_op_src.src = arg[1].value.uval;
            } else if (arg[1].type == ARGUMENT_TYPE_SYMBOL) {
                instruction.arithmetic_op_imm.address_type_bit = 1;
                instruction.arithmetic_op_imm.dst = arg[0].value.uval;
                symdata = symtable_get_symdata_by_name(context->symtable, line_content->args[1]);
                if (symdata == NULL) {
                    // TODO: Error
                } else {
                    reloc_table_add(context->reloctable, symdata->index, context->location_counter);
                    instruction.arithmetic_op_imm.imm = 0;
                }
            }
            break;

        case OP_AND:
        case OP_OR:
        case OP_NOT:
        case OP_TEST:
            READ_ARGS(2);
            if (arg[0].type == ARGUMENT_TYPE_REGISTER && arg[1].type == ARGUMENT_TYPE_REGISTER) {
                instruction.logical_op.dst = arg[0].value.uval;
                instruction.logical_op.src = arg[1].value.uval;
            } else {
                // TODO: Handle errors
            }
            break;

        case OP_LDR:
        //case OP_STR:
            READ_ARGS(4);
            if (arg[0].type == ARGUMENT_TYPE_REGISTER && arg[1].type == ARGUMENT_TYPE_REGISTER) {
                if (arg[1].value.uval != AS_REGISTER_PSW && arg[1].value.uval < AS_REGISTER_GENERAL_END) {
                    // TODO: error
                }

                if (arg[2].type == ARGUMENT_TYPE_SYMBOL) {
                    instruction.load_store_op.imm = 0;
                    symdata = symtable_get_symdata_by_name(context->symtable, line_content->args[2]);
                    if (symdata == NULL) {
                        // TODO: Error
                    } else {
                        reloc_table_add(context->reloctable, symdata->index, context->location_counter);
                    }
                } else if (arg[2].type == ARGUMENT_TYPE_IMMEDIATE) {
                    instruction.load_store_op.imm = arg[2].value.ival;
                } else {
                    // TODO: error
                }

                if (arg[3].type == ARGUMENT_TYPE_EXTRA) {
                    instruction.load_store_op.f = arg[3].value.uval;
                } else if(arg[3].type == ARGUMENT_TYPE_NONE) {
                    instruction.load_store_op.f = EXTRA_REG_FUNCTION_NONE;
                } else {
                    // TODO: error
                }

                // first two params
                instruction.load_store_op.r = arg[0].value.uval;
                instruction.load_store_op.a = arg[1].value.uval;
            } else {
                // TODO: Handle errors
            }
            break;

        case OP_CALL:
            READ_ARGS(2);
            if (arg[0].type != ARGUMENT_TYPE_REGISTER ||
                (arg[1].type != ARGUMENT_TYPE_IMMEDIATE && arg[1].type != ARGUMENT_TYPE_SYMBOL)) {
                // TODO: handle errors
            } else {
                instruction.call_op.dst = arg[0].value.uval;

                if (arg[1].type == ARGUMENT_TYPE_SYMBOL) {
                    symdata = symtable_get_symdata_by_name(context->symtable, line_content->args[1]);
                    if (symdata == NULL) {
                        // TODO: Error
                    } else {
                        reloc_table_add(context->reloctable, symdata->index, context->location_counter);
                        instruction.arithmetic_op_imm.imm = 0;
                    }
                } else {
                    instruction.call_op.imm = arg[1].value.ival;
                }
            }

            break;

        case OP_IN:
        //case OP_OUT:
            READ_ARGS(2);
            if (arg[0].type == ARGUMENT_TYPE_REGISTER && arg[1].type == ARGUMENT_TYPE_REGISTER) {
                uint32_t  val = arg[0].value.uval;
                instruction.io_op.dst = val;
                instruction.io_op.src = arg[1].value.uval;
            }
            break;

        case OP_MOV:
        //case OP_SHR:
        //case OP_SHL:
            READ_ARGS(3);
            if (arg[0].type == ARGUMENT_TYPE_REGISTER
                    && arg[1].type == ARGUMENT_TYPE_REGISTER
                    && (arg[2].type == ARGUMENT_TYPE_SYMBOL
                        || arg[2].type == ARGUMENT_TYPE_IMMEDIATE
                        || arg[2].type == ARGUMENT_TYPE_NONE)) {
                instruction.mov_op.dst = arg[0].value.uval;
                instruction.mov_op.src = arg[1].value.uval;

                if (arg[2].type == ARGUMENT_TYPE_SYMBOL) {
                    symdata = symtable_get_symdata_by_name(context->symtable, line_content->args[2]);
                    if (symdata == NULL) {
                        // TODO: Error
                    } else {
                        reloc_table_add(context->reloctable, symdata->index, context->location_counter);
                        instruction.mov_op.imm = 0;
                    }
                } else if (arg[2].type == ARGUMENT_TYPE_IMMEDIATE) {
                    instruction.mov_op.imm = arg[2].value.uval;
                } else {
                    instruction.mov_op.imm = 0;
                }
            }

            break;

        case OP_LDCH:
        //case OP_LDCL:
            READ_ARGS(2);
            if (arg[0].type == ARGUMENT_TYPE_REGISTER
                && (arg[1].type == ARGUMENT_TYPE_IMMEDIATE || arg[1].type == ARGUMENT_TYPE_SYMBOL)) {
                instruction.ldlh_op.dst = arg[0].value.uval;

                if (arg[1].type == ARGUMENT_TYPE_SYMBOL) {
                    symdata = symtable_get_symdata_by_name(context->symtable, line_content->args[1]);
                    if (symdata == NULL) {
                        // TODO: Error
                    } else {
                        reloc_table_add(context->reloctable, symdata->index, context->location_counter);
                        instruction.mov_op.imm = 0;
                    }
                } else {
                    instruction.ldlh_op.c = arg[1].value.ival;
                }
            }
            break;

        default:
            break;
    }
    return instruction;
}

void read_operation(union instruction* instruction_ptr, const char *name_str) {
    for (uint32_t i = 0; i < INSTRUCTION_COUNT; i += 1) {
        for (uint32_t operation = 0; operation < OPERATION_CONDITION_END; operation += 1) {
            if (strutil_consists_of(name_str, instruction_info[i].name, condition_info[operation])) {
                instruction_ptr->instruction.opcode = instruction_info[i].opcode;
                instruction_ptr->instruction.cf = instruction_info[i].cf;
                instruction_ptr->instruction.condition = operation;

                // special cases
                // load/store bit
                if (instruction_ptr->instruction.opcode == OP_LDR) {
                    if (strutil_begins_with(name_str, "ldr")) {
                        instruction_ptr->load_store_op.ls = MEMORY_FUNCTION_LOAD;
                    } else {
                        instruction_ptr->load_store_op.ls = MEMORY_FUNCTION_STORE;
                    }
                } else if (instruction_ptr->instruction.opcode == OP_IN) {
                    if (strutil_begins_with(name_str, "in")) {
                        instruction_ptr->io_op.io = IO_BIT_INPUT;
                    } else {
                        instruction_ptr->load_store_op.ls = IO_BIT_OUTPUT;
                    }
                } else if (instruction_ptr->instruction.opcode == OP_MOV) {
                    if (strutil_begins_with(name_str, "shr")) {
                        instruction_ptr->mov_op.lr = SHIFT_DIRECTION_RIGHT;
                    } else if (strutil_begins_with(name_str, "shl")) {
                        instruction_ptr->mov_op.lr = SHIFT_DIRECTION_LEFT;
                    } else {
                        instruction_ptr->mov_op.imm = 0;
                    }
                } else if (instruction_ptr->instruction.opcode == OP_LDCH) {
                    if (strutil_begins_with(name_str, "ldch")) {
                        instruction_ptr->ldlh_op.hl = PART_BYTE_HIGHER;
                    } else {
                        instruction_ptr->ldlh_op.hl = PART_BYTE_LOWER;
                    }
                }

                return;
            }
        }
    }
}

argument_info_t
read_argument(const char* arg_str) {
    int i = 0;
    size_t arglen = strlen(arg_str);
    value_t value;

    if (arglen == 0) {
        value.uval = 0;
        return (argument_info_t){ value, ARGUMENT_TYPE_NONE };
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
        if (reg_num > AS_REGISTER_GENERAL_END || reg_num < AS_REGISTER_GENERAL_BEGIN)
            goto ret_error;
        value.uval = (uint32_t)reg_num;
        return (argument_info_t){ value, ARGUMENT_TYPE_REGISTER };
    }
    // check if argument is immediate value, +|-(num...)
    //
    else if (((arg_str[0] == '+' || arg_str[0] == '-') && arglen > 1)
            || isdigit(arg_str[0])){
        for (i = 1; i < strlen(arg_str); i += 1) {
            if (!isdigit(arg_str[i])) {
                goto ret_error;
            }
        }
        int32_t imm_val = atoi(arg_str);
        value.ival = imm_val;
        return (argument_info_t){ value, ARGUMENT_TYPE_IMMEDIATE };
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
        value.uval = 0;
        return (argument_info_t){ value, ARGUMENT_TYPE_SYMBOL };
    }
ret_error:
    value.uval = 0;
    return (argument_info_t){ value, ARGUMENT_TYPE_ERROR };
}

argument_info_t
check_extra(const char* arg_str) {
    value_t value = { 0 };
    argument_info_t argument = { value, ARGUMENT_TYPE_EXTRA };
    if (strutil_is_equal(arg_str, EXTRA_FUNCTION_POST_INC))
        argument.value.uval = EXTRA_REG_FUNCTION_POST_INC;
    else if (strutil_is_equal(arg_str, EXTRA_FUNCTION_PRE_INC))
        argument.value.uval = EXTRA_REG_FUNCTION_PRE_INC;
    else if (strutil_is_equal(arg_str, EXTRA_FUNCTION_PRE_DEC))
        argument.value.uval = EXTRA_REG_FUNCTION_PRE_DEC;
    else if (strutil_is_equal(arg_str, EXTRA_FUNCTION_POST_DEC))
        argument.value.uval = EXTRA_REG_FUNCTION_POST_DEC;
    else
        argument.type = ARGUMENT_TYPE_ERROR;
    return argument;
}

argument_info_t
check_register(const char* arg_str) {
    value_t value = { 0 };
    argument_info_t argument = { value, ARGUMENT_TYPE_REGISTER };
    if (strutil_is_equal(arg_str, AS_REGISTER_PC_STR))
        argument.value.uval = AS_REGISTER_PC;
    else if (strutil_is_equal(arg_str, AS_REGISTER_SP_STR))
        argument.value.uval = AS_REGISTER_SP;
    else if (strutil_is_equal(arg_str, AS_REGISTER_LR_STR))
        argument.value.uval = AS_REGISTER_LR;
    else if (strutil_is_equal(arg_str, AS_REGISTER_PSW_STR))
        argument.value.uval = AS_REGISTER_PSW;
    else
        argument.type = ARGUMENT_TYPE_ERROR;
    return argument;
}