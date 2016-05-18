/** \file instruction.h
 *
 * \brief holds instruction structures and various bit/byte values.
 */

#ifndef NIKI_ASM_INSTRUCTION_H
#define NIKI_ASM_INSTRUCTION_H

#include <stdint.h>

#define INSTRUCTION_SIZE 4
/**
 * 4 byte union that holds all possible instruction variations, for easy read/write.
 */
union inst_t {
    struct {
        uint32_t off:8;
        uint32_t src:4;
        uint32_t nu:20;
    } int_op_t;

    struct {
        uint32_t off:8;
        uint32_t dst:5;
        uint32_t address_type_bit:1;
        uint32_t src:5;
        uint32_t nu:13;
    } arithmetic_op_src;

    struct {
        uint32_t off:8;
        uint32_t dst:5;
        uint32_t address_type_bit:1;
        uint32_t imm:18;
    } arithmetic_op_imm;

    struct {
        uint32_t off:8;
        uint32_t dst:5;
        uint32_t src:5;
        uint32_t nu:14;
    } logical_op;

    struct {
        uint32_t off:8;
        uint32_t a:5;
        uint32_t r:5;
        uint32_t f:3;
        uint32_t ls:1;
        uint32_t imm:10;
    } load_store_op;

    struct {
        uint32_t off:8;
        uint32_t dst:5;
        uint32_t imm:19;
    } call_op;

    struct {
        uint32_t off:8;
        uint32_t dst:4;
        uint32_t src:4;
        uint32_t io:1;
        uint32_t nu:15;
    } io_op;

    struct {
        uint32_t off:8;
        uint32_t dst:5;
        uint32_t src:5;
        uint32_t imm:5;
        uint32_t lr:1;
        uint32_t nu:8;
    } mov_op;

    struct {
        uint32_t off:8;
        uint32_t dst:4;
        uint32_t hl:1;
        uint32_t nu:3;
        uint32_t c:16;
    } ldlh_op;

    struct {
        uint32_t condition:3;
        uint32_t cf:1;
        uint32_t opcode:4;
        uint32_t operands:24; // max val
    } instruction;
};

/**
 * enum addressing_mode
 */
enum addressing_mode {
    ADDRESSING_MODE_DIRECT = 0,
    ADDRESSING_MODE_IMMEDIATE = 1,
};

/**
 * enum io_bit
 * input output instruction bit, used for in, out instructions
 */
enum io_bit {
    IO_BIT_OUTPUT = 0,
    IO_BIT_INPUT = 1,
};

/**
 * enum shift
 * shift direction for MOV, SHR, SHL
 */
enum shift_direction {
    SHIFT_DIRECTION_RIGHT = 0,
    SHIFT_DIRECTION_LEFT = 1,
};

/**
 * enum part_byte
 */
enum part_byte {
    PART_BYTE_LOWER = 0,
    PART_BYTE_HIGHER = 1,
};

/**
 * enum extra_reg_function
 *
 * extra register function for load and store operations
 */
enum extra_reg_function {
    EXTRA_REG_FUNCTION_NONE = 0,
    EXTRA_REG_FUNCTION_POST_INC = 2,
    EXTRA_REG_FUNCTION_POST_DEC = 3,
    EXTRA_REG_FUNCTION_PRE_INC = 4,
    EXTRA_REG_FUNCTION_PRE_DEC = 5,
};

/**
 * enum memory_function
 *
 * specifies which procedure is performed on given memory location
 */
enum memory_function {
    MEMORY_FUNCTION_STORE = 0,
    MEMORY_FUNCTION_LOAD = 1,
};

enum opcodes {
    OP_INT = 0x0,
    OP_ADD = 0x1,
    OP_SUB = 0x2,
    OP_MUL	 = 0x3,
    OP_DIV	 = 0x4,
    OP_CMP	 = 0x5,
    OP_AND	 = 0x6,
    OP_OR	 = 0x7,
    OP_NOT	 = 0x8,
    OP_TEST  = 0x9,
    OP_LDR = 0xA,
    OP_STR	= 0xB,
    OP_CALL  = 0xC,
    OP_IN = 0xD,
    OP_OUT = 0xD,
    OP_MOV = 0xE,
    OP_SHR = 0xE,
    OP_SHL = 0xE,
    OP_LDCH = 0xF,
    OP_LDCL = 0xF,
    OPCODES_END = 0x10,
};

enum operation_conditions {
    EQ = 00,
    NE = 01,
    GT = 02,
    GE = 03,
    LT = 04,
    LE = 05,
    UNUSED = 06,
    NONE = 07,
    OPERATION_CONDITIONS_END = 010,
};

#endif //NIKI_ASM_INSTRUCTION_H
