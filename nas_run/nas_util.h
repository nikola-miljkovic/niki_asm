//
// Created by nikola-miljkovic on 5/31/16.
//

#ifndef NIKI_ASM_NAS_UTIL_H
#define NIKI_ASM_NAS_UTIL_H

#include <stdio.h>
#include <stdint.h>

struct nas_context {
    uint8_t*    text_section;
    uint8_t*    data_section;
    uint8_t*    bss_section;

    size_t      text_size;
    size_t      data_size;
    size_t      bss_size;

    struct sym_table*   symtable;
    struct reloc_table* reloctable;
};

size_t nas_util_get_file_size(FILE* fptr);
struct nas_context *nas_util_load_context(FILE* fptr, size_t fsize);
void nas_util_destroy_context(struct nas_context **context);

#endif //NIKI_ASM_NAS_UTIL_H
