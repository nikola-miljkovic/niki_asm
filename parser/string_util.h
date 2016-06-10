//
// Created by nikola-miljkovic on 5/18/16.
//

#ifndef NIKI_ASM_STRING_UTIL_H
#define NIKI_ASM_STRING_UTIL_H

#include <stdint.h>

// fast equal check
int32_t strutil_is_equal(const char* str1, const char* str2);
int32_t strutil_consists_of(const char* str1, const char* part1, const char* part2);
int32_t strutil_is_empty(const char* str1);
int32_t strutil_begins_with(const char* str1, const char* str2);
char *strutil_trim(char *str);

enum string_types {
    STRING_TYPE_ALIGN,
    STRING_TYPE_NUMBER,
    STRING_TYPE_SYMBOL,
    STRING_TYPE_UNKNOWN,
};

int32_t check_type(char* str);
char    *get_align_symbol(char* arg);
uint32_t get_align_number(char* arg);

#endif //NIKI_ASM_STRING_UTIL_H
