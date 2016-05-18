#include <stdlib.h>
#include <string.h>
#include <parser/parser.h>
#include <ctype.h>

#include "as.h"

struct symtable_t*
symtable_create()
{
    struct symtable_t* symtable = malloc(sizeof(struct symtable_t));
    symtable->value = NULL;
    symtable->next = NULL;
    return symtable;
}

void
symtable_destroy(struct symtable_t **symtable_ptr)
{
    struct symtable_t* element = *symtable_ptr;
    struct symtable_t* next_element = NULL;

    while (element != NULL) {
        // TODO: Check if we need to de-alloc name
        // free symbol data first
        free(element->value);
        next_element = element->next;

        // now free symtable element
        free(element);
        element = next_element;
    }

    *symtable_ptr = NULL;
}

int
symtable_add_symbol(
        struct symtable_t *t,
        char *name,
        uint8_t section,
        uint8_t scope,
        uint8_t type,
        uint32_t offset,
        uint32_t size)
{
    struct symtable_t* last_element = t;
    struct symtable_t* new_element = NULL;

    /* check if first element is last element */
    if (last_element->value == NULL) {
        new_element = t;
        new_element->value = malloc(sizeof(struct symdata_t));
        new_element->value->index = 0;
        goto write_and_return;
    }

    /* go to last element and check if symbol with this name already exists */
    while (last_element->next != NULL) {
        last_element = last_element->next;
        if (strcmp(last_element->value->name, name) == 0) {
            return -1;
        }
    }

    /* create new symtable element, and set last elements next to new element */
    new_element = malloc(sizeof(struct symtable_t));
    last_element->next = new_element;

    /* set values of new element */
    new_element->next = NULL;
    new_element->value = malloc(sizeof(struct symdata_t));
    new_element->value->index = last_element->value->index + 1;

    // writes all other data and returns success code(0)
write_and_return:
    strcpy(new_element->value->name, name);
    new_element->value->section = section;
    new_element->value->scope = scope;
    new_element->value->type = type;
    new_element->value->offset = offset;
    new_element->value->size = size;
    return 0;
}

struct symdata_t*
symtable_get_symdata(struct symtable_t* symtable, uint32_t index) {
    struct symtable_t* current = symtable;

    while (current != NULL) {
        if (current->value->index == index) {
            return current->value;
        }
        current = current->next;
    }

    return NULL;
}

struct symdata_t*
symtable_get_symdata_by_name(struct symtable_t *symtable, char *name) {
    struct symtable_t* current = symtable;

    while (current != NULL) {
        if (strcmp(current->value->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }

    return NULL;
}

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


union inst_t
get_instruction(const line_content_t* line_content) {
    union inst_t instruction;

    return instruction;
}

argument_info_t
read_argument(char* arg_str) {
    int i = 0;
    // check if it is valid register string
    // if so return reg_number else return -1
    // format is R|r<0-15>\0
    //
    if ((arg_str[0] == 'r' || arg_str[0] == 'R') && strlen(arg_str) > 1) {
        for (i = 1; i < strlen(arg_str); i += 1) {
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
    else if (((arg_str[0] == '+' || arg_str[0] == '-') && strlen(arg_str) > 1)
            || isdigit(arg_str[0])){
        for (i = 1; i < strlen(arg_str); i += 1) {
            if (!isdigit(arg_str[i])) {
                goto ret_error;
            }
        }
        int32_t imm_val = atoi(arg_str);
        return (argument_info_t){ imm_val, ARGUMENT_TYPE_IMMEDIATE };
    }
    // check if argument is symbolname or extra func
    //
    else {
        // check if argument is postinc preinc postdec predec
        //
        argument_info_t extra = check_extra(arg_str);
        if (extra.type == ARGUMENT_TYPE_EXTRA)
            return extra;

        // check if argument is symbolname
        //
        for (i = 0; i < strlen(arg_str); i += 1) {
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
check_extra(char* arg_str) {
    if (strcmp(arg_str, EXTRA_FUNCTION_POST_INC) == 0)
        return (argument_info_t){ EXTRA_REG_FUNCTION_POST_INC, ARGUMENT_TYPE_EXTRA };
    if (strcmp(arg_str, EXTRA_FUNCTION_PRE_INC) == 0)
        return (argument_info_t){ EXTRA_REG_FUNCTION_PRE_INC, ARGUMENT_TYPE_EXTRA };
    if (strcmp(arg_str, EXTRA_FUNCTION_PRE_DEC) == 0)
        return (argument_info_t){ EXTRA_REG_FUNCTION_PRE_DEC, ARGUMENT_TYPE_EXTRA };
    if (strcmp(arg_str, EXTRA_FUNCTION_POST_DEC) == 0)
        return (argument_info_t){ EXTRA_REG_FUNCTION_POST_DEC, ARGUMENT_TYPE_EXTRA };
}