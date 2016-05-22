#include <stdlib.h>
#include "reloc.h"
#include "as.h"

struct reloc_table_t*
reloc_table_create() {
    struct reloc_table_t* reloctable = malloc(sizeof(struct reloc_table_t));
    reloctable->next = NULL;
    reloctable->value = NULL;
    return reloctable;
}

void reloc_table_destroy(struct reloc_table_t** table) {
    struct reloc_table_t* element = *table;
    struct reloc_table_t* next_element = NULL;

    while (element != NULL) {
        free(element->value);
        next_element = element->next;

        free(element);
        element = next_element;
    }

    *table = NULL;
}

void reloc_table_add(struct reloc_table_t* table, uint32_t index, uint32_t offset) {
    struct reloc_table_t* last_element = table;
    struct reloc_table_t* new_element = NULL;

    if (last_element->value == NULL) {
        new_element = table;
        goto write_and_return;
    }

    while (last_element->next != NULL) {
        last_element = last_element->next;
    }

    new_element = malloc(sizeof(struct reloc_table_t));
    last_element->next = new_element;

write_and_return:
    new_element->value = malloc(sizeof(struct reloc_data_t));
    new_element->value->index = index;
    new_element->value->offset = offset;
}

uint32_t
reloc_table_get_offset(struct reloc_table_t *table, uint32_t index) {
    struct reloc_table_t* current = table;

    while (current != NULL) {
        if (current->value->index == index) {
            return current->value->offset;
        }
        current = current->next;
    }

    return 0;
}

struct reloc_table_t* reloc_table_create_from_buffer(const uintptr_t* buffer) {
    return NULL;
}

void reloc_table_dump_to_buffer(const struct reloc_table_t* table, uintptr_t* buffer) {

}