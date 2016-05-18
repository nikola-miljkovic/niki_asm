//
// Created by nikola-miljkovic on 5/18/16.
//

#ifndef NIKI_ASM_STRING_UTIL_H
#define NIKI_ASM_STRING_UTIL_H

#include <stdint.h>

// fast equal check
int32_t strutil_is_equal(char* str1, char* str2);
int32_t strutil_consists_of(char* str1, char* part1, char* part2);

#endif //NIKI_ASM_STRING_UTIL_H
