//
// Created by nikola-miljkovic on 5/18/16.
//

#include <string.h>
#include "string_util.h"

int32_t strutil_is_equal(char *str1, char *str2) {
    size_t len = strlen(str1);
    if (len != strlen(str2))
        return 0;

    for (int i = 0; i < len; i += 1) {
        if (str1[i] != str2[i])
            return 0;
    }

    return 1;
}

int32_t strutil_consists_of(char *str1, char *part1, char *part2) {
    size_t len = strlen(str1);
    if (len != (strlen(part1) + strlen(part2))) {
        return 0;
    }
    return 0;
}



