//
// Created by nikola-miljkovic on 6/5/16.
//

#include <stdint.h>
#include <nas/instruction.h>

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

void write_psw_to_reg(int32_t* psw_reg, psw_t psw);
psw_t get_psw(int32_t* psw_reg);

int32_t read_int(uint8_t* buffer_start, size_t size);

#endif //NIKI_ASM_EMULATOR_H
