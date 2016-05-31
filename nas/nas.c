/**
 * nas - Niki assembler
 *
 * Created by Nikola Miljkovic <milja1337@gmail.com>
 */

#include <stdio.h>
#include <string.h>
#include <parser/parser.h>
#include <parser/string_util.h>

#include "nas.h"
#include "instruction.h"
#include "as.h"
#include "parser/parser.h"
#include "symbol.h"
#include "reloc.h"

/* Entry-point */
int main(int argc, char *argv[])
{
#if DEBUG
    printf("NOTE: Debugging enabled!\n");
#endif
    ASSERT_AND_EXIT(argc == 1, "ERROR: No input file specified.\n");

    printf("NOTE: Size of instruction %lu", sizeof(union instruction));

    char input_file[512];
    char output_file[512] = "";

    // check params, skip first
    for (int i = 1; i < argc; i++) {
        // check set output filename
        if (strcmp(argv[i], "-o") == 0) {
            ASSERT_AND_EXIT(i + 1 >= argc, "ERROR: Output filename not specified\n");
            strcpy(output_file, argv[++i]);
            continue;
        }

        strcpy(input_file, argv[i]);
    }

    FILE* fp_input = fopen(input_file, "r");
    ASSERT_AND_EXIT(fp_input == NULL, "ERROR: Input file '%s' doesn't exist.\n", input_file);

    // if output file isn't specified set it to be "<input_file>.o"
    if (strlen(output_file) == 0) {
        strcat(output_file, input_file);
        strcat(output_file, ".o");
    }
    FILE* fp_output = fopen(output_file, "w+");
    ASSERT_AND_EXIT(fp_output == NULL, "ERROR: Output file '%s' doesn't exist.\n", output_file);

#if DEBUG
    printf("DEBUG: Parsing file '%s'\n", input_file);
#endif

    line_content_t program_lines[MAX_PROGRAM_SIZE];
    int parse_status = parse_file(fp_input, program_lines, MAX_PROGRAM_SIZE);
    ASSERT_AND_EXIT(parse_status < 0, "ERROR: Compilation aborted.\n");

    // create symtable and put default sections
    struct sym_table* symtable = symtable_create();
    symtable_add_symbol(symtable, SECTION_NAME_TEXT, SYMBOL_SECTION_TEXT, SYMBOL_SCOPE_LOCAL, SYMBOL_TYPE_SECTION, 0, 0);
    symtable_add_symbol(symtable, SECTION_NAME_BSS, SYMBOL_SECTION_BSS, SYMBOL_SCOPE_LOCAL, SYMBOL_TYPE_SECTION, 0, 0);
    symtable_add_symbol(symtable, SECTION_NAME_DATA, SYMBOL_SECTION_DATA, SYMBOL_SCOPE_LOCAL, SYMBOL_TYPE_SECTION, 0, 0);

    // create empty reloc table
    struct reloc_table* reloc = reloc_table_create();

    // create elf context
    struct elf_context context = {
            symtable, reloc
    };

    /* Initiate location counters and size */
    uint8_t current_section = SYMBOL_SECTION_NONE;
    uint32_t location_counter[SYMBOL_SECTION_END] = { 0 };
    int32_t size = 0;

    /*
     *  first compilation iteration
     *  Here we analyze non instruction symbols and update location counters
     *
     *  After first iteration we will get all symbols defintions, extern/public
     *  And exact sizes of each section
     */
    for (int i = 0; i < MAX_PROGRAM_SIZE && strcmp(program_lines[i].name, SECTION_NAME_END) != 0; i++) {
        switch (program_lines[i].type) {
            case LINE_TYPE_DIRECTIVE:
            {
                size = get_directive_size(&program_lines[i]);
                ASSERT_AND_EXIT(size < 0, "ERROR: Compilation failed at line (%d): Unknown directive '%s'\n", i, program_lines[i].name);
                if (size == 0) {
                    // additional checks
                    if (strutil_is_equal(program_lines[i].name, SECTION_NAME_TEXT))
                        current_section = SYMBOL_SECTION_TEXT;
                    else if (strutil_is_equal(program_lines[i].name, SECTION_NAME_DATA))
                        current_section = SYMBOL_SECTION_DATA;
                    else if (strutil_is_equal(program_lines[i].name, SECTION_NAME_BSS))
                        current_section = SYMBOL_SECTION_BSS;
                    else if (strutil_is_equal(program_lines[i].name, DIRECTIVE_NAME_EXTERN)) {
                        // extern symbol
                        for (int32_t arg_i = 0; arg_i < MAX_INSTRUCTON_ARGUMENTS; arg_i += 1) {
                            if (!strutil_is_empty(program_lines[i].args[arg_i])) {
                                int status = symtable_add_symbol(symtable,
                                                                 program_lines[i].args[arg_i],
                                                                 SYMBOL_SECTION_NONE,
                                                                 SYMBOL_SCOPE_GLOBAL,
                                                                 SYMBOL_TYPE_EXTERN,
                                                                 0, 0);
                                ASSERT_AND_EXIT(status < 0,
                                                "ERROR: Compilation failed at line (%d): Symbol '%s' is already defined\n",
                                                i, program_lines[i].label);
                            } else {
                                ASSERT_AND_EXIT(arg_i == 0,
                                                "ERROR: Compilation failed at line (%d): Invalid directive arguments\n",
                                                i);
                                break;
                            }
                        }
                    }
                }

                location_counter[current_section] += size;
            } break;
            case LINE_TYPE_INSTRUCTION:
            {
                ASSERT_AND_EXIT(current_section != SYMBOL_SECTION_TEXT, "ERROR: Compilation failed at line (%d): Instruction outside of text section\n", i);

                size = INSTRUCTION_SIZE;
                location_counter[current_section] += size;
            } break;
            case LINE_TYPE_LABEL:
            {
                ASSERT_AND_EXIT(!strutil_is_empty(program_lines[i + 1].label), "ERROR: Compilation failed at line (%d): Double label\n", i);

                strcpy(program_lines[i + 1].label, program_lines[i].label);
                continue;
            }
            default:
                break;
        }

        if (strlen(program_lines[i].label) > 0) {
            int status = symtable_add_symbol(symtable, program_lines[i].label,
                                             current_section,
                                             SYMBOL_SCOPE_LOCAL,
                                             current_section == SYMBOL_SECTION_TEXT ? SYMBOL_TYPE_FUNCTION : SYMBOL_TYPE_DATA,
                                             location_counter[current_section],
                                             size > 0 ? (uint32_t)size : 0);
            ASSERT_AND_EXIT(status < 0, "ERROR: Compilation failed at line (%d): Symbol '%s' is already defined\n", i, program_lines[i].label);
        }
    }

    // init buffers and reset location counters
    uint8_t* binary_buffer[SYMBOL_SECTION_END];
    for (int i = 0; i < SYMBOL_SECTION_END; i++) {
        binary_buffer[i] = malloc(location_counter[i]*sizeof(uint8_t));
        location_counter[i] = 0;
    }
    current_section = SYMBOL_SECTION_NONE;

    /*
     *  Second iteration
     *  We compile each instruction and write into output buffers
     *  We execute directives like public
     *
     *  Bss section usese buffer but is not written to a file...
     *
     */
    for (int i = 0; i < MAX_PROGRAM_SIZE && strcmp(program_lines[i].name, SECTION_NAME_END) != 0; i++) {
        context.location_counter = location_counter[current_section];
        switch (program_lines[i].type) {
            case LINE_TYPE_DIRECTIVE:
            {
                size = get_directive_size(&program_lines[i]);
                ASSERT_AND_EXIT(size < 0, "ERROR: Compilation failed at line (%d): Unknown directive '%s'\n", i, program_lines[i].name);
                if (size == 0) {
                    // additional checks
                    if (strcmp(program_lines[i].name, SECTION_NAME_TEXT) == 0)
                        current_section = SYMBOL_SECTION_TEXT;
                    else if (strcmp(program_lines[i].name, SECTION_NAME_DATA) == 0)
                        current_section = SYMBOL_SECTION_DATA;
                    else if (strcmp(program_lines[i].name, SECTION_NAME_BSS) == 0)
                        current_section = SYMBOL_SECTION_BSS;
                    else if (strutil_is_equal(program_lines[i].name, DIRECTIVE_NAME_PUBLIC)) {
                        for (int32_t arg_i = 0; arg_i < MAX_INSTRUCTON_ARGUMENTS; arg_i += 1) {
                            if (!strutil_is_empty(program_lines[i].args[arg_i])) {
                                struct sym_entry* node = symtable_get_symdata_by_name(symtable, program_lines[i].args[arg_i]);
                                ASSERT_AND_EXIT(node == NULL,
                                                "ERROR: Compilation failed at line (%d): Symbol '%s' is not defined\n",
                                                i, program_lines[i].args[arg_i]);
                                node->scope = SYMBOL_SCOPE_GLOBAL;
                            } else {
                                ASSERT_AND_EXIT(arg_i == 0,
                                                "ERROR: Compilation failed at line (%d): Invalid directive arguments\n", i);
                                break;
                            }
                        }
                    }
                } else {
                    int32_t arg;
                    size_t value_size = (size_t)size;
                    if (strcmp(program_lines[i].name, DIRECTIVE_NAME_SKIP) == 0) {
                        arg = 0;
                    } else if (strcmp(program_lines[i].name, DIRECTIVE_NAME_ALIGN) == 0) {
                        arg = 0;
                    } else {
                        arg = atoi(program_lines[i].args[0]);
                    }
                    memcpy(binary_buffer[current_section] + location_counter[current_section],
                           &arg, value_size);
                    location_counter[current_section] += value_size;
                }
            } break;
            case LINE_TYPE_INSTRUCTION:
            {
                size = INSTRUCTION_SIZE;
                union instruction instruction = get_instruction(&context, &program_lines[i]);
                memcpy(binary_buffer[current_section] + location_counter[current_section],
                       &instruction, (size_t)size);
                location_counter[current_section] += size;
            } break;
            default:
                break;
        }
    }

    // create symtable buffer and dump data to it
    size_t symtable_size = symtable->length * sizeof(struct sym_entry);
    uint8_t* symtable_buffer = malloc(symtable_size);
    symtable_dump_to_buffer(symtable, symtable_buffer);

    // create reloc table buffer and dump data to it
    size_t reloctable_size = context.reloctable->length * sizeof(struct reloc_entry);
    uint8_t* reloctable_buffer = malloc(reloctable_size);
    reloc_table_dump_to_buffer(context.reloctable, reloctable_buffer);

    /* Create output data and write it! */
    struct elf output;
    uint32_t elf_size = sizeof(struct elf);

    output.bss_size = location_counter[SYMBOL_SECTION_BSS];
    output.data_start = elf_size;
    output.text_start = output.data_start + location_counter[SYMBOL_SECTION_DATA];
    output.symbol_start = output.text_start + location_counter[SYMBOL_SECTION_TEXT];
    output.reloc_start = output.symbol_start + (uint32_t)symtable_size;
    output.size = output.reloc_start + (uint32_t)reloctable_size;

    uint8_t* output_buffer = malloc(elf_size + output.size);

    memcpy(output_buffer, &output, elf_size);
    memcpy(output_buffer + output.data_start, binary_buffer[SYMBOL_SECTION_DATA], location_counter[SYMBOL_SECTION_DATA]);
    memcpy(output_buffer + output.text_start, binary_buffer[SYMBOL_SECTION_TEXT], location_counter[SYMBOL_SECTION_TEXT]);
    memcpy(output_buffer + output.symbol_start, symtable_buffer, symtable_size);
    memcpy(output_buffer + output.reloc_start, reloctable_buffer, reloctable_size);

    fwrite(output_buffer, sizeof(uint8_t), elf_size + output.size, fp_output);

    fclose(fp_output);
    fclose(fp_input);
    return 0;
}



