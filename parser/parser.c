#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"
#include "string_util.h"

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

int parse_script(char* file_name, script_content_t *script_content) {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    int i = 0;

    fp = fopen(file_name, "r");
    if (fp != NULL) {
        for(;(read = getline(&line, &len, fp)) != -1; i += 1) {
            script_content[i].type = SCRIPT_CONTENT_NONE;

            if (strlen(line) == 0) {
                continue;
            }

            for (int j = 0; j < strlen(line); j+=1) {
                if (line[j] == '\n') {
                    line[j] = '\0';
                }
            }

            if (line[0] == '.' && isalpha(line[1])) {
                // this is section
                strcpy(script_content[i].name, line + 1);
                script_content[i].type = SCRIPT_CONTENT_SECTION;
            } else {
                // this is not section
                size_t last_position = 0;
                size_t argument = 0;
                size_t operator = 0;

                for (int j = 0; j < strlen(line); j+=1) {
                    if (line[j] == '=') {
                        strncpy(script_content[i].name, line, (size_t)j);
                        strncpy(script_content[i].name,
                                strutil_trim(script_content[i].name),
                                j - last_position - 1);
                        script_content[i].type = SCRIPT_CONTENT_EXPRESSION;
                        last_position = (size_t)j;
                    } else {
                        if (line[j] == '+' || line[j] == '-') {
                            strncpy(script_content[i].args[argument++], line + last_position + 1, j - last_position - 1);
                            char* trimmed = strutil_trim(script_content[i].args[argument - 1]);

                            for (int k = 0; k <= strlen(trimmed); k += 1) {
                                script_content[i].args[argument - 1][k] = trimmed[k];
                            }

                            script_content[i].op[operator++] = line[j];
                            last_position = (size_t)j;
                        }
                    }
                }

                if (script_content[i].type == SCRIPT_CONTENT_EXPRESSION) {
                    strncpy(script_content[i].args[argument++], line + last_position + 1, strlen(line) - last_position - 1);
                    char* trimmed = strutil_trim(script_content[i].args[argument - 1]);

                    for (int k = 0; k <= strlen(trimmed); k += 1) {
                        script_content[i].args[argument - 1][k] = trimmed[k];
                    }
                }

                script_content[i].arguments = argument;
                script_content[i].operators = operator;
            }
        }
    }

    fclose(fp);
    return i;
}

int isdot(int v) { return v == '.'; }
int iscoma(int v) { return v == ','; }
int isalnumsymbol(int v) { return (isalnum(v) || v == '+' || v == '-'); }
int iscomaorminus(int v) { return (iscoma(v) || v == '-'); }








