//
// Created by nikola-miljkovic on 5/18/16.
//

#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include "string_util.h"
#include "parser.h"

int32_t strutil_is_equal(const char *str1, const char *str2) {
    size_t len = strlen(str1);
    if (len != strlen(str2))
        return 0;

    for (int i = 0; i < len; i += 1) {
        if (str1[i] != str2[i])
            return 0;
    }

    return 1;
}

int32_t strutil_consists_of(const char *str1, const char *part1, const char *part2) {
    size_t len = strlen(str1);
    size_t part1_len = strlen(part1);

    if (len != (part1_len + strlen(part2))) {
        return 0;
    }

    char current_character;
    for (int i = 0; i < len; i += 1) {
        current_character = i < part1_len ? part1[i] : part2[i - part1_len];

        if (str1[i] != current_character)
            return 0;
    }

    return 1;
}

int32_t strutil_is_empty(const char* str1) {
    size_t len = strlen(str1);
    return len == 0;
}

int32_t strutil_begins_with(const char* str1, const char* str2) {
    size_t lenpre = strlen(str1);
    size_t lenstr = strlen(str2);
    return lenstr < lenpre ? false : strncmp(str1, str2, lenpre) == 0;
}

char *strutil_trim(char* str) {
    char *end;

    // Trim leading space
    while (isspace(*str)) str++;

    if (*str == 0)  // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) end--;

    // Write new null terminator
    *(end + 1) = 0;

    return str;
}

int32_t check_type(char* str) {
    int32_t string_type = STRING_TYPE_UNKNOWN;

    // TODO: Proper implementation
    if (strutil_begins_with("align(", str)) {
        string_type = STRING_TYPE_ALIGN;
    } else if (isdigit(str[0])) {
        string_type = STRING_TYPE_NUMBER;
    } else if (isalnum(str[0]) || strutil_is_equal(".", str)) {
        string_type = STRING_TYPE_SYMBOL;
    }

    return string_type;
}

char *get_align_symbol(char* arg) {
    char *align_symbol;

    while(*arg != '(') arg++;

    align_symbol = arg + 1;

    while(*arg != ',') arg++;

    *arg = '\0';

    return align_symbol;
}

uint32_t get_align_number(char* arg) {
    while(*arg != ',') arg++;
    arg++;

    *(strlen(arg) + arg) = '\0';

    return (uint32_t)atoi(arg);
}
