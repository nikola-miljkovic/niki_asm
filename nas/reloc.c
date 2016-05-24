#include <stdlib.h>
#include <string.h>

#include "reloc.h"

struct reloc_table*
reloc_table_create() {
    struct reloc_table* reloctable = malloc(sizeof(struct reloc_table));
    reloctable->length = 0;
    reloctable->head = malloc(sizeof(struct reloc_node));
    reloctable->head->next = NULL;
    reloctable->head->value = NULL;
    return reloctable;
}

void reloc_table_destroy(struct reloc_table** table) {
    struct reloc_node* element = (*table)->head;
    struct reloc_node* next_element = NULL;

    while (element != NULL) {
        free(element->value);
        next_element = element->next;

        free(element);
        element = next_element;
    }

    *table = NULL;
}

void reloc_table_add(struct reloc_table* table, uint32_t index, uint32_t offset) {
    struct reloc_node* last_element = table->head;
    struct reloc_node* new_element = NULL;

    if (last_element->value == NULL) {
        new_element = table->head;
        goto write_and_return;
    }

    while (last_element->next != NULL) {
        last_element = last_element->next;
    }

    new_element = malloc(sizeof(struct reloc_table));
    last_element->next = new_element;

write_and_return:
    new_element->value = malloc(sizeof(struct reloc_entry));
    new_element->value->index = index;
    new_element->value->offset = offset;
    table->length += 1;
}

uint32_t
reloc_table_get_offset(struct reloc_table *table, uint32_t index) {
    struct reloc_node* current = table->head;

    while (current != NULL) {
        if (current->value->index == index) {
            return current->value->offset;
        }
        current = current->next;
    }

    return 0;
}

struct reloc_table* reloc_table_create_from_buffer(uint8_t* buffer) {
    return NULL;
}

void reloc_table_dump_to_buffer(const struct reloc_table* table, uint8_t* buffer) {
    struct reloc_node* current = table->head;
    const size_t reloc_entry_size = sizeof(struct reloc_entry);
    uint32_t offset = 0;

    while (current != NULL) {
        memcpy(buffer + offset, current->value, reloc_entry_size);
        offset += reloc_entry_size;

        current = current->next;
    }
}