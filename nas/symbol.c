#include <stdlib.h>
#include <string.h>

#include "symbol.h"

struct symtable_t*
symtable_create()
{
    struct symtable_t* symtable = malloc(sizeof(struct symtable_t));

    symtable->length = 0;
    symtable->head = malloc(sizeof(struct symtable_node_t));
    symtable->head->value = NULL;
    symtable->head->next = NULL;

    return symtable;
}

void symtable_destroy(struct symtable_t **symtable_ptr)
{
    struct symtable_node_t* element = (*symtable_ptr)->head;
    struct symtable_node_t* next_element = NULL;

    while (element != NULL) {
        // free symbol data first
        free(element->value);
        next_element = element->next;

        // now free symtable element
        free(element);
        element = next_element;
    }

    free(*symtable_ptr);
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
    struct symtable_node_t* last_element = t->head;
    struct symtable_node_t* new_element = NULL;

    /* check if first element is last element */
    if (last_element->value == NULL) {
        new_element = t->head;
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
    t->length += 1;
    return 0;
}

struct symdata_t*
symtable_get_symdata(struct symtable_t* symtable, uint32_t index) {
    struct symtable_node_t* current = symtable->head;

    while (current != NULL) {
        if (current->value->index == index) {
            return current->value;
        }
        current = current->next;
    }

    return NULL;
}

struct symdata_t*
symtable_get_symdata_by_name(struct symtable_t *symtable, const char *name) {
    struct symtable_node_t* current = symtable->head;

    while (current != NULL) {
        if (strcmp(current->value->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }

    return NULL;
}

void symtable_dump_to_buffer(struct symtable_t* symtable, uint8_t* buffer) {
    const size_t symdata_size = sizeof(struct symdata_t);
    struct symtable_node_t* current = symtable->head;
    uint32_t offset = 0;

    while (current != NULL) {
        memcpy(buffer + offset, current->value, symdata_size);

        offset += symdata_size;
        current = current->next;
    }
}
