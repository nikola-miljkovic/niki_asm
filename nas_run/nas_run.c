//
// Created by nikola-miljkovic on 5/24/16.
//

#include <stdio.h>
#include <stdlib.h>
#include <nas/symbol.h>
#include <nas/reloc.h>
#include <limits.h>
#include <string.h>
#include <nas/nas.h>

#include "nas_run.h"
#include "nas_util.h"
#include "nas/instruction.h"

static union instruction end_instruction = {
        .instruction.opcode = OP_CALL,
        .instruction.cf = 0,
        .instruction.condition = NONE,
        .call_op.dst = 0,
        .call_op.imm = INT16_MAX,
};

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printf("Error\n");
        exit(0);
    }

    /* Linking part */
    struct nas_context **context = malloc((argc - 1) * sizeof(struct nas_context*));
    size_t bss_size = 0;
    size_t data_size = 0;
    size_t text_size = 0;

    for (int i = 1; i < argc; i += 1) {
        char* input_file_name = argv[i];
        FILE* fptr = fopen(input_file_name, "rb");

        /* get file size */
        size_t file_size = nas_util_get_file_size(fptr);

        /* Load input fptr to buffer */
        context[i - 1] = nas_util_load_context(fptr, file_size);

        bss_size += context[i - 1]->bss_size;
        data_size += context[i - 1]->data_size;
        text_size += context[i - 1]->text_size;
    }

    /* Increase text size for one instruction, needed for exit command */
    text_size += sizeof(union instruction);

    struct nas_context *final_context = malloc(sizeof(struct nas_context));

    final_context->bss_section = calloc(0, bss_size);
    final_context->data_section = malloc(data_size);
    final_context->text_section = malloc(text_size);

    final_context->symtable = symtable_create();
    final_context->reloctable = reloc_table_create();

    /* Init offsets/counters needed for advancing through final_context */
    size_t bss_offset = 0;
    size_t data_offset = 0;
    size_t text_offset = 0;
    size_t symbol_offset = 0;

    /* Fill final context */
    for (int i = 0; i < argc - 1; i += 1) {
        /*
         * Resolve symbols, start from 3, skip BSS, DATA, TEXT
         * Do this before we update offsets!
         */
        int j = 0;
        for (struct sym_node* node = context[i]->symtable->head; node != NULL; node = node->next) {
            if (j != 3) {
                j += 1;
                continue;
            }

            struct sym_entry *entry = node->value;
            size_t offset_update;
            switch(entry->section) {
                case SYMBOL_SECTION_TEXT:
                    offset_update = text_offset;
                    break;
                case SYMBOL_SECTION_DATA:
                    offset_update = data_offset;
                    break;
                case SYMBOL_SECTION_BSS:
                    offset_update = bss_offset;
                    break;
                default:
                    offset_update = 0;
                    break;
            }

            /* add all symbols */
            symtable_add_symbol_force(final_context->symtable,
                                         entry->name,
                                         entry->section,
                                         entry->scope,
                                         entry->type,
                                         entry->offset + (uint32_t)offset_update,
                                         entry->size);

            symbol_offset += 1;
        }

        /* handle relocations, update indexes and offsets */
        uint32_t symbol_offset_current = i > 0 ? (uint32_t)symbol_offset : 0;
        for (struct reloc_node* node = context[i]->reloctable->head; node != NULL; node = node->next) {
            struct reloc_entry *entry = node->value;
            reloc_table_add(final_context->reloctable, entry->index + symbol_offset_current - 3*(i + 1), entry->offset + (uint32_t)text_offset);
        }

        if (context[i]->bss_size > 0) {
            memcpy(final_context->bss_section + bss_offset, context[i]->bss_section, context[i]->bss_size);
            bss_offset += context[i]->bss_size;
        }

        if (context[i]->data_size > 0) {
            memcpy(final_context->data_section + data_offset, context[i]->data_section, context[i]->data_size);
            data_offset += context[i]->data_size;
        }

        if (context[i]->text_size > 0) {
            memcpy(final_context->text_section + text_offset, context[i]->text_section, context[i]->text_size);
            text_offset += context[i]->text_size;

            /* We assume first context contains main, fill it with end instruction */
            if (i == 0) {
                memcpy(final_context->text_section + text_offset, &end_instruction, sizeof(union instruction));
                text_offset += sizeof(union instruction);
            }
        }
    }

    int32_t globals_resolved = symtable_resolve_globals(final_context->symtable);
    ASSERT_AND_EXIT(globals_resolved < 0, "LINKER ERROR: Cannot resolve global labels, some are defined more then once.");
}