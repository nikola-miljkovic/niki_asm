//
// Created by nikola-miljkovic on 5/18/16.
//

#ifndef NIKI_ASM_RELOC_H
#define NIKI_ASM_RELOC_H

#include <stdint.h>

struct reloc_entry {
    uint32_t index;
    uint32_t offset;
};

struct reloc_node {
    struct reloc_node* next;
    struct reloc_entry* value;
};

struct reloc_table {
    struct reloc_node* head;
    uint32_t    length;
};

struct reloc_table* reloc_table_create();
void reloc_table_destroy(struct reloc_table** table);

void reloc_table_add(struct reloc_table* table, uint32_t index, uint32_t offset);
uint32_t reloc_table_get_offset(struct reloc_table *table, uint32_t index);

struct reloc_table* reloc_table_create_from_buffer(uint8_t* buffer);
void reloc_table_dump_to_buffer(const struct reloc_table* table, uint8_t* buffer);

#endif //NIKI_ASM_RELOC_H
