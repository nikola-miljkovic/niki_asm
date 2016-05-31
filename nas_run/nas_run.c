//
// Created by nikola-miljkovic on 5/24/16.
//

#include <stdio.h>
#include <nas/symbol.h>

#include "nas_run.h"
#include "nas_util.h"

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        printf("Error");
    }

    char* input_file_name = argv[1];
    FILE* file = fopen(input_file_name, "rb");

    /* get file size */
    size_t file_size = nas_util_get_file_size(file);

    /* Load input file to buffer */
    struct nas_context *context = nas_util_load_context(file, file_size);

    symtable_destroy(&context->symtable);
}