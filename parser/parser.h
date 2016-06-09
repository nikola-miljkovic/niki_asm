#ifndef NIKI_ASM_PARSER_H
#define NIKI_ASM_PARSER_H

#include <stdio.h>
#include <stdint.h>

#define MAX_LINE_SIZE               1024
#define MAX_PROGRAM_SIZE            10000
#define MAX_INSTRUCTON_ARGUMENTS    4
#define MAX_LABEL_SIZE              32

#define COMMENT_CHARACTER           ';'
#define END_OF_LABEL_CHARACTER      ':'

enum line_type {
    LINE_TYPE_UNDEFINED,
    LINE_TYPE_DIRECTIVE,
    LINE_TYPE_LABEL,
    LINE_TYPE_INSTRUCTION,
    LINE_TYPE_END,
};

typedef struct {
    uint8_t type;
    char label[MAX_LABEL_SIZE];
    char name[MAX_LABEL_SIZE];
    char args[MAX_INSTRUCTON_ARGUMENTS][MAX_LABEL_SIZE];
} line_content_t;

typedef struct {
    uint8_t type;
    char name[MAX_LABEL_SIZE];
    char args[MAX_INSTRUCTON_ARGUMENTS][MAX_LABEL_SIZE];
    char op[MAX_INSTRUCTON_ARGUMENTS];
    size_t arguments;
    size_t operators;
} script_content_t;

enum script_content_type {
    SCRIPT_CONTENT_EXPRESSION,
    SCRIPT_CONTENT_SECTION,
    SCRIPT_CONTENT_NONE,
};

typedef int(*test_function_t)(int);

/* need to check directive */
int isdot(int v);
int iscoma(int v);
int isalnumsymbol(int v);
int iscomaorminus(int v);


int parse_file(FILE *fp, line_content_t *program_lines, size_t program_lines_length);
int read_label(const char* line, line_content_t* line_content);
int read_directive(const char* line, line_content_t* line_content);
int read_instruction(const char* line, line_content_t* line_content);

int parse_script(char* file_name, script_content_t *script_content);

#endif //NIKI_ASM_PARSER_H
