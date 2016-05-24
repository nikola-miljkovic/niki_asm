//
// Created by nikola-miljkovic on 5/18/16.
//

#include <string.h>
#include "string_util.h"

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
    return len > 0;
}





