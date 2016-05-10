#include <stdlib.h>
#include <string.h>

#include "as.h"

struct symtable_t*
symtable_create() {
    struct symtable_t* symtable = malloc(sizeof(struct symtable_t));
    symtable->value = NULL;
    symtable->next = NULL;
    return symtable;
}

void
symtable_destroy(struct symtable_t **symtable_ptr) {
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
        uint32_t size) {
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
        if (strcmp(last_element->value->name, name) == 0) {
            return -1;
        }

        last_element = last_element->next;
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
    new_element->value->name = name;
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



