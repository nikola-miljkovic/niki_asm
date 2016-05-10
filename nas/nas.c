/**
 * nas - Niki assembler
 *
 * Created by Nikola Miljkovic <milja1337@gmail.com>
 */

#include <stdio.h>
#include <string.h>

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

    struct symtable_t* symtable = symtable_create();
    symtable_add_symbol(symtable, SECTION_NAME_TEXT, SYMBOL_SECTION_TEXT, SYMBOL_SCOPE_LOCAL, SYMBOL_TYPE_SECTION, 0, 0);
    symtable_add_symbol(symtable, SECTION_NAME_BSS, SYMBOL_SECTION_BSS, SYMBOL_SCOPE_LOCAL, SYMBOL_TYPE_SECTION, 0, 0);
    symtable_add_symbol(symtable, SECTION_NAME_DATA, SYMBOL_SECTION_DATA, SYMBOL_SCOPE_LOCAL, SYMBOL_TYPE_SECTION, 0, 0);

    fclose(fp_output);
    fclose(fp_input);
    return 0;
}



