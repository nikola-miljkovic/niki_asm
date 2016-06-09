//
// Created by nikola-miljkovic on 6/5/16.
//

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <parser/string_util.h>
#include "emulator.h"

void write_psw_to_reg(int32_t* psw_reg, psw_t psw) {
    memcpy(psw_reg, &psw, sizeof(psw_t));
}

psw_t get_psw(int32_t* psw_reg) {
    psw_t psw;
    memcpy(&psw, psw_reg, sizeof(psw_t));
    return psw;
}

int32_t read_int(uint8_t* buffer_start, size_t size) {
    int8_t int8;
    int16_t int16;
    int32_t int32;

    switch (size) {
        case 1:
            memcpy(&int8, buffer_start, size);
            return int8;
        case 2:
            memcpy(&int16, buffer_start, size);
            return int16;
        case 4:
            memcpy(&int32, buffer_start, size);
            return int32;
        default:
            return 0;
    }
}

script_global_table_t *create_script_global() {
    script_global_table_t* table = malloc(sizeof(script_global_table_t));
    table->next = NULL;
    table->node = NULL;
    return table;
}

void add_script_global(script_global_table_t* table, char* name, int32_t value) {
    script_global_table_t* local_table = table;
    if (local_table->node == NULL) {
        local_table->next = NULL;
        local_table->node = malloc(sizeof(script_global_t));
        strcpy(local_table->node->name, name);
        local_table->node->value = value;
    } else {
        script_global_node_t* node = local_table;

        while(node->next != NULL && !strutil_is_equal(node->node->name, name)) {
            node = node->next;
        }

        if (strutil_is_equal(node->node->name, name)) {
            node->node->value = value;
            return;
        }

        node->next = malloc(sizeof(script_global_table_t));
        node = node->next;
        node->next = NULL;
        node->node = malloc(sizeof(script_global_t));
        strcpy(node->node->name, name);
        node->node->value = value;
    }
}

script_global_t *get_script_global(script_global_table_t* table, char* name) {
    script_global_node_t* node = table;

    while(node != NULL) {
        if (strutil_is_equal(node->node->name, name)) {
            return node->node;
        }

        node = node->next;
    }

    return NULL;
}