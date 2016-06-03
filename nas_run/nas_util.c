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

    const size_t text_size = elf_data.symbol_start - elf_data.text_start;
    const size_t data_size = elf_data.text_start - elf_data.data_start;

    /* create nas_context allocate and fill */
    struct nas_context* context = malloc(sizeof(struct nas_context));
    context->bss_section = calloc(0, elf_data.bss_size);

    context->data_section = malloc(data_size);
    memcpy(context->data_section, buffer + elf_data.data_start, data_size);

    context->text_section = malloc(text_size);
    memcpy(context->text_section, buffer + elf_data.text_start, text_size);

    context->symtable = symtable_create_from_buffer(buffer + elf_data.symbol_start, elf_data.reloc_start - elf_data.symbol_start);
    context->reloctable = reloc_table_create_from_buffer(buffer + elf_data.reloc_start, elf_data.size - elf_data.reloc_start);

    context->bss_size = elf_data.bss_size;
    context->data_size = data_size;
    context->text_size = text_size;

    return context;
}

void nas_util_destroy_context(struct nas_context **context) {
    struct nas_context *contextptr = *context;
    free(contextptr->bss_section);
    free(contextptr->data_section);
    free(contextptr->text_section);

    symtable_destroy(&contextptr->symtable);
    reloc_table_destroy(&contextptr->reloctable);

    free(contextptr);
    context = NULL;
}