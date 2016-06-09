//
// Created by nikola-miljkovic on 6/5/16.
//

#include <stdint.h>
#include <nas/instruction.h>
#include <parser/parser.h>

#ifndef NIKI_ASM_EMULATOR_H
#define NIKI_ASM_EMULATOR_H

#define ENTRY_POINT_LABEL "main"
#define MAX_USER_MEMORY  5000
#define IVT_ENTRIES      16

typedef struct {
    uint32_t z:1;
    uint32_t o:1;
    uint32_t c:1;
    uint32_t n:1;
    uint32_t off:27;
    uint32_t mask:1;
} psw_t;

typedef union {
    int32_t intv;
    int16_t ints[2];
} int_lh;

void write_psw_to_reg(int32_t* psw_reg, psw_t psw);
psw_t get_psw(int32_t* psw_reg);

int32_t read_int(uint8_t* buffer_start, size_t size);

/* Script related */
typedef struct {
    char    name[MAX_LABEL_SIZE];
    int32_t value;
} script_global_t;

typedef struct script_global_table {
    script_global_t *node;
    struct script_global_table *next;
} script_global_table_t;

typedef script_global_table_t script_global_node_t;

script_global_table_t *create_script_global();
void add_script_global(script_global_table_t* table, char* name, int32_t value);
script_global_t *get_script_global(script_global_table_t* table, char* name);


#endif //NIKI_ASM_EMULATOR_H
