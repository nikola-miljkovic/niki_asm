#ifndef NIKI_ASM_PARSER_H
#define NIKI_ASM_PARSER_H

#include <stdio.h>

#define MAX_LINE_SIZE               1024
#define MAX_PROGRAM_SIZE            10000
#define MAX_DIRECTIVE_ARGUMENTS     10
#define MAX_INSTRUCTON_ARGUMENTS    2
#define MAX_LABEL_SIZE              32

#define COMMENT_CHARACTER           ';'

enum line_type {
    LINE_TYPE_UNDEFINED,
    LINE_TYPE_DIRECTIVE,
    LINE_TYPE_LABEL,
    LINE_TYPE_INSTRUCTION,
    LINE_TYPE_END,
};

struct line_content_t {
    uint8_t type;
    char label[MAX_LABEL_SIZE];
    char* name;
    char* args[MAX_INSTRUCTON_ARGUMENTS];
};

void parse_file(FILE* fp);

#endif //NIKI_ASM_PARSER_H
