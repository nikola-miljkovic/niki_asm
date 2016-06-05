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
#include <nas/instruction.h>

#include "nas_run.h"
#include "nas_util.h"
#include "nas/instruction.h"
#include "emulator.h"

static union instruction end_instruction = {
        .instruction.opcode = OP_CALL,
        .instruction.cf = 0,
        .instruction.condition = NONE,
        .call_op.dst = 0,
        .call_op.imm = INT16_MAX,
};

static int32_t registers[AS_REGISTER_END] = { 0 };
static uint8_t* memory_buffer;
//static uint32_t ivt_table[IVT_ENTRIES];

int main(int argc, char *argv[]) {
    if (argc <= 2) {
        printf("Error\n");
        exit(0);
    }

    /* Linking part */
    struct nas_context **context = malloc((argc - 2) * sizeof(struct nas_context*));
    size_t bss_size = 0;
    size_t data_size = 0;
    size_t text_size = 0;

    /* first is for script */
    for (int i = 0; i < argc - 2; i += 1) {
        char* input_file_name = argv[i + 2];
        FILE* fptr = fopen(input_file_name, "rb");

        /* get file size */
        size_t file_size = nas_util_get_file_size(fptr);

        /* Load input fptr to buffer */
        context[i] = nas_util_load_context(fptr, file_size);

        bss_size += context[i]->bss_size;
        data_size += context[i]->data_size;
        text_size += context[i]->text_size;
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
    for (int i = 0; i < argc - 2; i += 1) {
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

    final_context->text_size = text_offset;
    final_context->data_size = data_offset;
    final_context->bss_size = bss_offset;

    /* TODO: Add globals from script */

    int32_t globals_resolved = symtable_resolve_globals(final_context->symtable);
    ASSERT_AND_EXIT(globals_resolved < 0, "LINKER ERROR: Cannot resolve global labels, some are defined more then once.");

    /* Create memory */
    memory_buffer = malloc(sizeof(uint8_t) * MAX_USER_MEMORY);
    uint32_t memory_location_start = 0;
    uint32_t memory_location = memory_location_start;
    uint32_t start_data = memory_location;
    uint32_t start_bss = 0;
    uint32_t start_text = 0;

    memcpy(memory_buffer + memory_location, final_context->data_section, final_context->data_size);
    memory_location += final_context->data_size;

    memcpy(memory_buffer + memory_location, final_context->bss_section, final_context->bss_size);
    start_bss = memory_location;
    memory_location += final_context->bss_size;

    memcpy(memory_buffer + memory_location, final_context->text_section, final_context->text_size);

    /* Find main label */
    struct sym_entry* main_label = symtable_get_symdata_by_name(final_context->symtable, ENTRY_POINT_LABEL);
    ASSERT_AND_EXIT(main_label == NULL, "EMULATOR ERROR: No main found, exiting.");

    /* pint pc onto main label */
    registers[AS_REGISTER_PC] = memory_location + main_label->offset;

    start_text = memory_location;
    memory_location += final_context->text_size;

    write_psw_to_reg(&registers[AS_REGISTER_PSW], (psw_t){ 0, 0, 0, 0, 0, 0 });
    registers[AS_REGISTER_SP] = memory_location;

    /* handle relocations */
    for (struct reloc_node *node = final_context->reloctable->head; node != NULL; node = node->next) {
        struct reloc_entry *entry = node->value;

        /* find index in symtable */
        struct sym_entry* symbol = symtable_get_symdata(final_context->symtable, entry->index);
        if (symbol == NULL) {
            // ERROR
        }

        union instruction instr;
        memcpy(&instr, memory_buffer + start_text + entry->offset, sizeof(union instruction));

        int32_t symbol_val;
        switch(instr.instruction.opcode) {
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
                ASSERT_AND_EXIT(symbol->type != SYMBOL_TYPE_DATA, "ERROR: Symbol is not data type.");
                if (symbol->section == SYMBOL_SECTION_DATA) {
                    symbol_val = read_int(memory_buffer + start_data + symbol->offset - symbol->size, symbol->size);
                } else { /*BSS_SECTION*/
                    symbol_val = read_int(memory_buffer + start_bss + symbol->offset, symbol->size);
                }
                instr.arithmetic_op_imm.imm = symbol_val;
                break;

            case OP_LDR:
            //case OP_STR
                ASSERT_AND_EXIT(symbol->type != SYMBOL_TYPE_DATA, "ERROR: Symbol is not data type.");
                if (symbol->section == SYMBOL_SECTION_DATA) {
                    symbol_val = read_int(memory_buffer + start_data + symbol->offset - symbol->size, symbol->size);
                } else { /*BSS_SECTION*/
                    symbol_val = read_int(memory_buffer + start_bss + symbol->offset, symbol->size);
                }
                instr.load_store_op.imm = symbol_val;
                break;

            case OP_CALL:
                ASSERT_AND_EXIT(symbol->type != SYMBOL_TYPE_FUNCTION, "ERROR: Symbol is not function type.");
                instr.call_op.imm = start_text + symbol->offset - symbol->size;
                break;

            case OP_MOV:
            //case OP_SHR:
            //case OP_SHL:
                ASSERT_AND_EXIT(symbol->type != SYMBOL_TYPE_DATA, "ERROR: Symbol is not data type.");
                if (symbol->section == SYMBOL_SECTION_DATA) {
                    symbol_val = read_int(memory_buffer + start_data + symbol->offset - symbol->size, symbol->size);
                } else { /*BSS_SECTION*/
                    symbol_val = read_int(memory_buffer + start_bss + symbol->offset, symbol->size);
                }
                instr.mov_op.imm = (uint32_t)symbol_val;
                break;

            default:
                ASSERT_AND_EXIT(1, "INVALID CODE");
                break;
        }

        /* write back to memory */
        memcpy(memory_buffer + start_text + entry->offset, &instr, sizeof(union instruction));
    }
}
