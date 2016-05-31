//
// Created by nikola-miljkovic on 5/31/16.
//

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <nas/as.h>
#include <nas/symbol.h>
#include <nas/reloc.h>

#include "nas_util.h"

size_t nas_util_get_file_size(FILE* fptr) {
    fseek (fptr , 0 , SEEK_END);
    int64_t size = ftell(fptr);
    rewind (fptr);
    return size <= 0 ? 0 : (size_t)size;
}

struct nas_context*
nas_util_load_context(FILE* fptr, size_t fsize) { ;
    uint8_t* buffer = malloc(sizeof(uint8_t) * fsize);
    fread(buffer, fsize, sizeof(uint8_t), fptr);

    /* load struct elf */
    struct elf elf_data;
    memcpy(&elf_data, buffer, sizeof(struct elf));

    /* create nas_context */
    struct nas_context* context = malloc(sizeof(struct nas_context));
    context->bss_section = malloc(elf_data.bss_size);
    context->data_section = malloc(elf_data.text_start - elf_data.data_start);
    context->text_section = malloc(elf_data.symbol_start - elf_data.text_start);
    context->symtable = symtable_create_from_buffer(buffer + elf_data.symbol_start, elf_data.reloc_start - elf_data.symbol_start);
    context->reloctable = reloc_table_create_from_buffer(buffer + elf_data.reloc_start, elf_data.size - elf_data.reloc_start);

    /* copy the content */
    return context;
}