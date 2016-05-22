//
// Created by nikola-miljkovic on 5/18/16.
//

#ifndef NIKI_ASM_RELOC_H
#define NIKI_ASM_RELOC_H

#include <stdint.h>
#include <stddef.h>

struct reloc_data_t {
    uint32_t index;
    uint32_t offset;
};

struct reloc_table_t {
    struct reloc_table_t* next;
    struct reloc_data_t* value;
};


struct reloc_table_t* reloc_table_create();
void reloc_table_destroy(struct reloc_table_t** table);

void reloc_table_add(struct reloc_table_t* table, uint32_t index, uint32_t offset);
uint32_t reloc_table_get_offset(struct reloc_table_t *table, uint32_t index);

struct reloc_table_t* reloc_table_create_from_buffer(const uintptr_t* buffer);
void reloc_table_dump_to_buffer(const struct reloc_table_t* table, uintptr_t* buffer);

#endif //NIKI_ASM_RELOC_H
