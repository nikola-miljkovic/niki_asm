#include <stdlib.h>
#include <string.h>

#include "symbol.h"

struct sym_table*
symtable_create()
{
    struct sym_table* symtable = malloc(sizeof(struct sym_table));

    symtable->length = 0;
    symtable->head = malloc(sizeof(struct sym_node));
    symtable->head->value = NULL;
    symtable->head->next = NULL;

    return symtable;
}

void symtable_destroy(struct sym_table **symtable_ptr)
{
    struct sym_node* element = (*symtable_ptr)->head;
    struct sym_node* next_element = NULL;

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
        struct sym_table *t,
        char *name,
        uint8_t section,
        uint8_t scope,
        uint8_t type,
        uint32_t offset,
        uint32_t size)
{
    struct sym_node* last_element = t->head;
    struct sym_node* new_element = NULL;

    /* check if first element is last element */
    if (last_element->value == NULL) {
        new_element = t->head;
        new_element->value = malloc(sizeof(struct sym_entry));
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
    new_element = malloc(sizeof(struct sym_table));
    last_element->next = new_element;

    /* set values of new element */
    new_element->next = NULL;
    new_element->value = malloc(sizeof(struct sym_entry));
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

struct sym_entry*
symtable_get_symdata(struct sym_table* symtable, uint32_t index) {
    struct sym_node* current = symtable->head;

    while (current != NULL) {
        if (current->value->index == index) {
            return current->value;
        }
        current = current->next;
    }

    return NULL;
}

struct sym_entry*
symtable_get_symdata_by_name(struct sym_table *symtable, const char *name) {
    struct sym_node* current = symtable->head;

    while (current != NULL) {
        if (strcmp(current->value->name, name) == 0) {
            return current->value;
        }
        current = current->next;
    }

    return NULL;
}

void symtable_dump_to_buffer(struct sym_table* symtable, uint8_t* buffer) {
    const size_t symdata_size = sizeof(struct sym_entry);
    struct sym_node* current = symtable->head;
    uint32_t offset = 0;

    while (current != NULL) {
        memcpy(buffer + offset, current->value, symdata_size);

        offset += symdata_size;
        current = current->next;
    }
}
