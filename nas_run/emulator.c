//
// Created by nikola-miljkovic on 6/5/16.
//

#include <string.h>
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