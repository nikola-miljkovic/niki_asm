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

#endif //NIKI_ASM_STRING_UTIL_H
