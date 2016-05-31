//
// Created by nikola-miljkovic on 5/31/16.
//

#ifndef NIKI_ASM_NAS_UTIL_H
#define NIKI_ASM_NAS_UTIL_H

#include <stdio.h>

struct nas_context {
    uint32_t*   text_section;
    uint8_t*    data_section;
    uint8_t*    bss_section;

    struct sym_table*   symtable;
    struct reloc_table* reloctable;
};

size_t nas_util_get_file_size(FILE* fptr);
struct nas_context* nas_util_load_context(FILE* fptr, size_t fsize);

#endif //NIKI_ASM_NAS_UTIL_H
