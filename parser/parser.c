#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"

int
parse_file(FILE *fp, line_content_t *program_lines, size_t program_lines_length)
{
    char current_line[MAX_LINE_SIZE];
    uint32_t line_number = 0;

    // loop specific
    int label_status;
    int directive_status;
    int instruction_status;

    while(fgets(current_line, MAX_LINE_SIZE, fp) != NULL) {
        if (line_number >= program_lines_length)
            return -1;

        program_lines[line_number].type = LINE_TYPE_UNDEFINED;

#if DEBUG
        printf("Parsing line (%u): %s", line_number + 1, current_line);
#endif

        // Parse each line and put in cooresponding program_lines[line_number]
        // until .end
        label_status = read_label(current_line, &program_lines[line_number]);
        directive_status = read_directive(current_line + (label_status > 0 ? label_status : 0), &program_lines[line_number]);
        instruction_status = read_instruction(current_line + (label_status > 0 ? label_status : 0), &program_lines[line_number]);

        if (directive_status == 0 && instruction_status == 0) {
            printf("ERROR: Compilation error, line (%u): %s", line_number + 1, current_line);
            return -1;
        } else if (directive_status == -1 && instruction_status == -1 && label_status > 0) {
            program_lines[line_number].type = LINE_TYPE_LABEL;
        } else if (directive_status == 0) {
            program_lines[line_number].type = LINE_TYPE_DIRECTIVE;
        } else if (instruction_status == 0) {
            program_lines[line_number].type = LINE_TYPE_INSTRUCTION;
        }

        //break_line:
        line_number++;
    }

    return 0;
}

int
read_label(const char* line, line_content_t* line_content)
{
    uint32_t position = 0;
    uint32_t last_test_position = 0;
    uint32_t current_func = 0;
    test_function_t test_func[] = {
            &isblank,
            &isalnum,
    };
    size_t vector_size = sizeof(test_func) / sizeof(test_function_t);

    while (line[position] != '\0' && line[position] != COMMENT_CHARACTER) {
        if (test_func[current_func](line[position])) {
            position++;
            continue;
        }

        // test function didn't pass? set next test function
        current_func += 1;
        if (current_func < vector_size) {
            last_test_position = position;
            position++;
            continue;
        } else {
            if (line[position] == END_OF_LABEL_CHARACTER) {
                strncpy(line_content->label, line + last_test_position, position - last_test_position);
                return position + 1;
            } else {
                return -1;
            }
        }
    };

    return -1;
}

int
read_directive(const char *line, line_content_t *line_content)
{
    uint32_t position = 0;
    uint32_t last_test_position = 0;
    uint32_t current_func = 0;
    uint32_t directive_arg = 0;
    test_function_t test_func[] = {
            &isblank,
            &isdot,         // start of directive
            &isalpha,       // actual directive
            &isblank,       // blankspace
            &isalnumsymbol,       // argument
            &iscoma,        // coma for arguments
    };

    size_t vector_size = sizeof(test_func) / sizeof(test_function_t);
    int ret_code = -1;

    while (line[position] != '\0' && line[position] != COMMENT_CHARACTER) {
        if (test_func[current_func](line[position])) {
            // if we located coma we shouldn't test for it but whitespace/alphanum instead
            if (test_func[current_func] == &iscoma) {
                current_func -= 2;
            }

            position++;
            continue;
        }

        // this happens if there is no dot found condition ***MUST*** happen once!
        if (position == last_test_position && test_func[current_func] == &isdot) {
            break;
        }

        // test function didn't pass? set next test function
        current_func += 1;
        if (current_func < vector_size) {
            // check if change is after .<directive>\s\t then write to line_content directive
            if (current_func >= 2 && position != last_test_position) {
                if (test_func[current_func - 1] == &isalpha && test_func[current_func - 2] == &isdot) {
                    strncpy(line_content->name, line + last_test_position, position - last_test_position);
                    ret_code = 0;
                } else if (test_func[current_func - 1] == &isalnumsymbol && test_func[current_func - 2] == &isblank) {
                    // read argument!
                    strncpy(line_content->args[directive_arg++], line + last_test_position, position - last_test_position);
                }
            }
            last_test_position = position;
        } else {
            break;
        }
    }

    return ret_code;
}

int
read_instruction(const char *line, line_content_t *line_content)
{
    uint32_t position = 0;
    uint32_t last_test_position = 0;
    uint32_t current_func = 0;
    uint32_t directive_arg = 0;
    test_function_t test_func[] = {
            &isblank,
            &isalpha,           // instruction
            &isblank,           // blankspace
            &isalnumsymbol,     // argument
            &iscoma,            // coma fora arguments
    };

    size_t vector_size = sizeof(test_func) / sizeof(test_function_t);
    int ret_code = -1;

    while (line[position] != '\0' && line[position] != COMMENT_CHARACTER) {
        if (test_func[current_func](line[position])) {
            // if we located coma we shouldn't test for it. but whitespace/alphanum instead
            if (test_func[current_func] == &iscoma) {
                current_func -= 2;
            }

            position++;
            continue;
        }

        // test function didn't pass? set next test function
        current_func += 1;
        if (current_func < vector_size) {
            // check if change is after instruction\s\t[args..] then write to line_content directive
            if (current_func >= 2 && position != last_test_position) {
                if (test_func[current_func - 1] == &isalpha && test_func[current_func - 2] == &isblank) {
                    strncpy(line_content->name, line + last_test_position, position - last_test_position);
                    ret_code = 0;
                } else if (test_func[current_func - 1] == &isalnumsymbol && test_func[current_func - 2] == &isblank) {
                    // read arguments!
                    strncpy(line_content->args[directive_arg++], line + last_test_position,
                            position - last_test_position);
                }
            }
            last_test_position = position;
        } else {
            break;
        }
    }

    return ret_code;
}

int isdot(int v) { return v == '.'; }
int iscoma(int v) { return v == ','; }
int isalnumsymbol(int v) { return (isalnum(v) || v == '+' || v == '-'); }
int iscomaorminus(int v) { return (iscoma(v) || v == '-'); }









