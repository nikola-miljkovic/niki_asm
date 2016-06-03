#include <stdlib.h>
#include <string.h>
#include <parser/string_util.h>

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
_symtable_add_symbol(
        struct sym_table *t,
        char *name,
        uint8_t section,
        uint8_t scope,
        uint8_t type,
        uint32_t offset,
        uint32_t size,
        uint32_t force)
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
        if (force == 0 && strcmp(last_element->value->name, name) == 0) {
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
    return _symtable_add_symbol(t, name, section, scope, type, offset, size, 0);
}

int
symtable_add_symbol_force(
        struct sym_table *t,
        char *name,
        uint8_t section,
        uint8_t scope,
        uint8_t type,
        uint32_t offset,
        uint32_t size)
{
    return _symtable_add_symbol(t, name, section, scope, type, offset, size, 1);
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

struct sym_table*
symtable_create_from_buffer(uint8_t* buffer, size_t size) {
    const size_t entry_size = sizeof(struct sym_entry);
    const size_t table_length = (uint32_t)size / entry_size;
    struct sym_table *table = symtable_create();
    struct sym_entry entry;


    for (int32_t i = 0; i < table_length; i += 1) {
        memcpy(&entry, buffer + i * entry_size, entry_size);
        symtable_add_symbol(table, entry.name, entry.section, entry.scope, entry.type, entry.offset, entry.size);
    }

    return table;
}

int symtable_resolve_globals(struct sym_table* symtable) {
    struct sym_node* current = symtable->head;

    while (current != NULL) {
        if (current->value->scope == SYMBOL_SCOPE_GLOBAL
            && current->value->section != SYMBOL_SECTION_NONE
            && current->value->type != SYMBOL_TYPE_EXTERN) {
            struct sym_node* current_inner = symtable->head;

            while(current_inner != NULL) {
                if (strutil_is_equal(current->value->name, current_inner->value->name)
                    && current_inner->value->scope == SYMBOL_SCOPE_GLOBAL) {
                    /* Check if its not updated and update it */
                    if (current_inner->value->section == SYMBOL_SECTION_NONE
                        && current_inner->value->type == SYMBOL_TYPE_EXTERN) {
                        current_inner->value->section = current->value->section;
                        current_inner->value->type = current->value->type;
                        current_inner->value->offset = current->value->offset;
                        current_inner->value->size = current->value->size;
                    /* Check if its same global, if not return -1 */
                    } else if (current_inner->value->section != current->value->section
                        || current_inner->value->type != current->value->type
                        || current_inner->value->offset != current->value->offset
                        || current_inner->value->size != current->value->size) {
                        return -1;
                    }
                }

                current_inner = current_inner->next;
            }
        }

        current = current->next;
    }

    return 0;
}