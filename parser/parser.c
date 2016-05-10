#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"

void parse_file(FILE *fp) {
    struct line_content_t program_lines[MAX_PROGRAM_SIZE];
    char current_line[MAX_LINE_SIZE];
    uint32_t line_number = 1;
    uint32_t position = 0;

    while(fgets(current_line, MAX_LINE_SIZE, fp) != NULL) {
        position = 0;
        program_lines[line_number].type = LINE_TYPE_UNDEFINED;

#if DEBUG
        printf("Parsing line (%u): %s", line_number, current_line);
#endif

        // Parse each line and put in cooresponding program_lines[line_number]
        // until .end
        while(1) {
            if (isspace(current_line[position])) {
                position++;
                continue;
            }

            // if it's only comment break this line and go to next
            if (current_line[position] == COMMENT_CHARACTER) {
                break;
            }

            if (isalnum(current_line[position])) {
                // could be label or instruction
                for (size_t i = 0; i < MAX_LINE_SIZE; i++) {
                    if (isalnum(current_line[position + i])) {
                        continue;
                    } else if (current_line[position + i] == ':') {
                        // we have label
                        strncpy(program_lines[line_number].label, current_line + position, i);
#if DEBUG
                        printf("DEBUG: Found label: %s", program_lines[line_number].label);
#endif
                        position += i;
                        goto break_line;
                    }
                }
            }

            if (current_line[position] == '.') {
                // advance pass '.' character
                position += 1;
                // parse directive
                for (size_t i = 0; i < MAX_LINE_SIZE; i++) {
                    char character = current_line[position + i];
                    if(isalpha(character)) {
                        continue;
                    } else if (isspace(character)){
                        strncpy(program_lines[line_number].name, current_line + position, i);
                        program_lines[line_number].type = LINE_TYPE_DIRECTIVE;
#if DEBUG
                        printf("DEBUG: Found directive: %s", program_lines[line_number].name);
#endif

                        // TODO: sanatize args
                        program_lines[line_number].args
                        goto break_line;
                    }
                }
            }
        }

        break_line:
        line_number++;
    }
}

