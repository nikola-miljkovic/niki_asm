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

/* Entry-point */
int main(int argc, char *argv[])
{
#if DEBUG
    printf("NOTE: Debugging enabled!\n");
#endif
    ASSERT_AND_EXIT(argc == 1, "ERROR: No input file specified.\n");

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
    struct symtable_t* symtable = symtable_create();
    symtable_add_symbol(symtable, SECTION_NAME_TEXT, SYMBOL_SECTION_TEXT, SYMBOL_SCOPE_LOCAL, SYMBOL_TYPE_SECTION, 0, 0);
    symtable_add_symbol(symtable, SECTION_NAME_BSS, SYMBOL_SECTION_BSS, SYMBOL_SCOPE_LOCAL, SYMBOL_TYPE_SECTION, 0, 0);
    symtable_add_symbol(symtable, SECTION_NAME_DATA, SYMBOL_SECTION_DATA, SYMBOL_SCOPE_LOCAL, SYMBOL_TYPE_SECTION, 0, 0);

    uint8_t current_section = SYMBOL_SECTION_NONE;
    uint32_t location_counter[SYMBOL_SECTION_END] = { 0 };
    int32_t size = 0;

    /* first compilation iteration */
    for (int i = 0; i < MAX_PROGRAM_SIZE && strcmp(program_lines[i].name, SECTION_NAME_END) != 0; i++) {
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
                }

                location_counter[current_section] += size;
            } break;
            case LINE_TYPE_INSTRUCTION:
            {
                size = INSTRUCTION_SIZE;
                location_counter[current_section] += size;
            } break;
            default:
                break;
        }

        if (strlen(program_lines[i].label) > 0) {
            int status = symtable_add_symbol(symtable, program_lines[i].label,
                                             current_section, SYMBOL_SCOPE_LOCAL, SYMBOL_TYPE_SECTION,
                                             location_counter[current_section], size > 0 ? (uint32_t)size : 0);
            ASSERT_AND_EXIT(status < 0, "ERROR: Compilation failed at line (%d): Symbol '%s' is already defined\n", i, program_lines[i].label);
        }
    }

    // init buffers and reset location counters
    uint8_t* binary_buffer[SYMBOL_SECTION_END];
    for (int i = 0; i < SYMBOL_SECTION_END; i++) {
        binary_buffer[i] = malloc(location_counter[i]);
        location_counter[i] = 0;
    }
    current_section = SYMBOL_SECTION_NONE;

    // second compilation iteration
    for (int i = 0; i < MAX_PROGRAM_SIZE && strcmp(program_lines[i].name, SECTION_NAME_END) != 0; i++) {
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
                } else {
                    int arg;
                    if (strcmp(program_lines[i].name, DIRECTIVE_NAME_SKIP) == 0) {
                        arg = 0;
                    } else if (strcmp(program_lines[i].name, DIRECTIVE_NAME_ALIGN) == 0) {
                        arg = 0;
                    } else {
                        arg = atoi(program_lines[i].args[0]);
                    }
                    memcpy(binary_buffer[current_section] + location_counter[current_section], &arg, (size_t)size);

                    location_counter[current_section] += size;
                }
            } break;
            case LINE_TYPE_INSTRUCTION:
            {
                size = INSTRUCTION_SIZE;
                location_counter[current_section] += size;
                get_instruction(&program_lines[i]);
            } break;
            default:
                break;
        }
    }

    fclose(fp_output);
    fclose(fp_input);
    return 0;
}



