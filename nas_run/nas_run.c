//
// Created by nikola-miljkovic on 5/24/16.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <nas/symbol.h>
#include <nas/reloc.h>
#include <limits.h>
#include <string.h>
#include <nas/nas.h>
#include <nas/instruction.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <parser/string_util.h>
#include <parser/parser.h>
#include <nas/as.h>

#include "nas_run.h"
#include "nas_util.h"
#include "nas/instruction.h"
#include "emulator.h"

static union instruction end_instruction;
static union instruction interrupt_routine_set_processor[4];
static union instruction on_key_pressed[9];

static int32_t registers[AS_REGISTER_END] = { 0 };
static uint8_t* memory_buffer;
static int32_t io_memory[0x4000];

void load_instructions();

int kbhit(void)
{
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if(ch != EOF)
    {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    load_instructions();

    if (argc <= 2) {
        printf("Error\n");
        exit(0);
    }

    /* Linking part */
    struct nas_context **context = malloc((argc - 2) * sizeof(struct nas_context*));
    size_t bss_size = 0;
    size_t data_size = 0;
    size_t text_size = 0;

    /* first is for script */
    for (int i = 0; i < argc - 2; i += 1) {
        char* input_file_name = argv[i + 2];
        FILE* fptr = fopen(input_file_name, "rb");

        /* get file size */
        size_t file_size = nas_util_get_file_size(fptr);

        /* Load input fptr to buffer */
        context[i] = nas_util_load_context(fptr, file_size);

        bss_size += context[i]->bss_size;
        data_size += context[i]->data_size;
        text_size += context[i]->text_size;
    }

    /* Increase text size for one instruction, needed for exit command */
    text_size += sizeof(union instruction);

    struct nas_context *final_context = malloc(sizeof(struct nas_context));

    final_context->bss_section = calloc(0, bss_size);
    final_context->data_section = malloc(data_size);
    final_context->text_section = malloc(text_size);

    final_context->symtable = symtable_create();
    final_context->reloctable = reloc_table_create();

    /* Init offsets/counters needed for advancing through final_context */
    size_t bss_offset = 0;
    size_t data_offset = 0;
    size_t text_offset = 0;
    size_t symbol_offset = 0;

    /* Fill final context */
    for (int i = 0; i < argc - 2; i += 1) {
        /*
         * Resolve symbols, start from 3, skip BSS, DATA, TEXT
         * Do this before we update offsets!
         */
        int j = 0;
        for (struct sym_node* node = context[i]->symtable->head; node != NULL; node = node->next) {
            if (j != 3) {
                j += 1;
                continue;
            }

            struct sym_entry *entry = node->value;
            size_t offset_update;
            switch(entry->section) {
                case SYMBOL_SECTION_TEXT:
                    offset_update = text_offset;
                    break;
                case SYMBOL_SECTION_DATA:
                    offset_update = data_offset;
                    break;
                case SYMBOL_SECTION_BSS:
                    offset_update = bss_offset;
                    break;
                default:
                    offset_update = 0;
                    break;
            }

            /* add all symbols */
            symtable_add_symbol_force(final_context->symtable,
                                         entry->name,
                                         entry->section,
                                         entry->scope,
                                         entry->type,
                                         entry->offset + (uint32_t)offset_update,
                                         entry->size);

            symbol_offset += 1;
        }

        /* handle relocations, update indexes and offsets */
        uint32_t symbol_offset_current = i > 0 ? (uint32_t)symbol_offset : 0;
        for (struct reloc_node* node = context[i]->reloctable->head; node != NULL; node = node->next) {
            struct reloc_entry *entry = node->value;
            reloc_table_add(final_context->reloctable, entry->index + symbol_offset_current - 3*(i + 1), entry->offset + (uint32_t)text_offset);
        }

        if (context[i]->bss_size > 0) {
            memcpy(final_context->bss_section + bss_offset, context[i]->bss_section, context[i]->bss_size);
            bss_offset += context[i]->bss_size;
        }

        if (context[i]->data_size > 0) {
            memcpy(final_context->data_section + data_offset, context[i]->data_section, context[i]->data_size);
            data_offset += context[i]->data_size;
        }

        if (context[i]->text_size > 0) {
            memcpy(final_context->text_section + text_offset, context[i]->text_section, context[i]->text_size);
            text_offset += context[i]->text_size;

            /* We assume first context contains main, fill it with end instruction */
            if (i == 0) {
                memcpy(final_context->text_section + text_offset, &end_instruction, sizeof(union instruction));
                text_offset += sizeof(union instruction);
            }
        }
    }

    final_context->text_size = text_offset;
    final_context->data_size = data_offset;
    final_context->bss_size = bss_offset;

    /* Create memory */
    memory_buffer = malloc(sizeof(uint8_t) * MAX_USER_MEMORY);
    script_content_t script_content[100];
    int script_len = parse_script(argv[1], script_content);

    uint32_t memory_location = 0;
    uint32_t start_ivt = 0;
    memory_location += start_ivt + sizeof(uint32_t) * IVT_ENTRIES;
    uint32_t start_data = memory_location;
    uint32_t start_bss = memory_location;
    uint32_t start_text = memory_location;

    script_global_table_t* global_table = create_script_global();
    add_script_global(global_table, ".", memory_location);

    for (int i = 0; i < script_len; i += 1) {
        /* update */
        if (script_content[i].type == SCRIPT_CONTENT_SECTION) {
            script_global_t* val = get_script_global(global_table, ".");
            memory_location = (uint32_t)val->value;

            if (strutil_is_equal(script_content[i].name, SECTION_NAME_TEXT)) {
                start_text = memory_location;
                memory_location += final_context->text_size;
            } else if (strutil_is_equal(script_content[i].name, SECTION_NAME_BSS)) {
                start_bss = memory_location;
                memory_location += final_context->bss_size;
            } else if (strutil_is_equal(script_content[i].name, SECTION_NAME_DATA)) {
                start_data = memory_location;
                memory_location += final_context->data_size;
            }

            add_script_global(global_table, ".", memory_location);
        } else if (script_content[i].type == SCRIPT_CONTENT_EXPRESSION) {
            /* calculate expression */
            /* check first */
            int32_t symbol_value = 0;
            int32_t argument_type = check_type(script_content[i].args[0]);

            // check if its align?
            if (argument_type == STRING_TYPE_SYMBOL) {
                script_global_t *val = get_script_global(global_table, script_content[i].args[0]);
                ASSERT_AND_EXIT(val == NULL, "Error: undefined symbol.");
                symbol_value = val->value;
            } else if (argument_type == STRING_TYPE_NUMBER) {
                symbol_value = atoi(script_content[i].args[0]);
            } else if (argument_type == STRING_TYPE_ALIGN) {
                uint32_t align_num = get_align_number(script_content[i].args[0]);
                char *align_symbol = get_align_symbol(script_content[i].args[0]);

                /* find symbol value */
                script_global_t *val = get_script_global(global_table, align_symbol);
                ASSERT_AND_EXIT(val == NULL, "Error: undefined symbol.");
                uint32_t mod = val->value % align_num;
                symbol_value = val->value + align_num - mod;
            }

            if (script_content[i].arguments == 1) {
                add_script_global(global_table, script_content[i].name, symbol_value);
                continue;
            }

            int32_t argument_value = 0;
            for (int j = 0; j < script_content[i].operators; j += 1) {
                argument_type = check_type(script_content[i].args[j + 1]);
                if (argument_type == STRING_TYPE_SYMBOL) {
                    script_global_t *val = get_script_global(global_table, script_content[i].args[j + 1]);
                    ASSERT_AND_EXIT(val == NULL, "Error: undefined symbol.");
                    argument_value = val->value;
                } else if (argument_type == STRING_TYPE_NUMBER) {
                    argument_value = atoi(script_content[i].args[j + 1]);
                }

                switch (script_content[i].op[j]) {
                    case '+':
                        symbol_value += argument_value;
                        break;
                    case '-':
                        symbol_value -= argument_value;
                        break;
                    default:
                        break;
                }
            }

            add_script_global(global_table, script_content[i].name, symbol_value);
        }
    }

    int32_t globals_resolved = symtable_resolve_globals(final_context->symtable);
    ASSERT_AND_EXIT(globals_resolved < 0, "LINKER ERROR: Cannot resolve global labels, some are defined more then once.\n");

    memcpy(memory_buffer + start_data, final_context->data_section, final_context->data_size);
    memcpy(memory_buffer + start_bss, final_context->bss_section, final_context->bss_size);
    memcpy(memory_buffer + start_text, final_context->text_section, final_context->text_size);

    /* Find main label */
    struct sym_entry* main_label = symtable_get_symdata_by_name(final_context->symtable, ENTRY_POINT_LABEL);
    ASSERT_AND_EXIT(main_label == NULL, "EMULATOR ERROR: No main found, exiting.\n");

    /* pint pc onto main label */
    registers[AS_REGISTER_PC] = start_text + main_label->offset - 4;

    /* handle relocations */
    for (struct reloc_node *node = final_context->reloctable->head; node != NULL; node = node->next) {
        int32_t symbol_val = 0;
        script_global_t* script_symbol = NULL;
        struct reloc_entry *entry = node->value;

        /* find index in symtable */
        struct sym_entry* symbol = symtable_get_symdata(final_context->symtable, entry->index);
        if (symbol == NULL) {
            ASSERT_AND_EXIT(1, "Unknown Symbol\n");
        } else if (symbol->section == SYMBOL_SECTION_NONE
                     && symbol ->type == SYMBOL_TYPE_EXTERN) {
            script_symbol = get_script_global(global_table, symbol->name);
            if (script_symbol == NULL) {
                // ERROR
            }
            symbol_val = script_symbol->value;
        }

        union instruction instr;
        memcpy(&instr, memory_buffer + start_text + entry->offset, sizeof(union instruction));

        switch(instr.instruction.opcode) {
            case OP_ADD:
            case OP_SUB:
            case OP_MUL:
            case OP_DIV:
                if (script_symbol == NULL) {
                    ASSERT_AND_EXIT(symbol->type != SYMBOL_TYPE_DATA, "ERROR: Symbol is not data type.\n");
                    if (symbol->section == SYMBOL_SECTION_DATA) {
                        symbol_val = read_int(memory_buffer + start_data + symbol->offset - symbol->size, symbol->size);
                    } else { /*BSS_SECTION*/
                        symbol_val = read_int(memory_buffer + start_bss + symbol->offset, symbol->size);
                    }
                }
                instr.arithmetic_op_imm.imm = symbol_val;
                break;

            case OP_LDR:
            //case OP_STR
                if (script_symbol == NULL) {
                    ASSERT_AND_EXIT(symbol->type != SYMBOL_TYPE_DATA, "ERROR: Symbol is not data type.\n");
                    if (symbol->section == SYMBOL_SECTION_DATA) {
                        symbol_val = read_int(memory_buffer + start_data + symbol->offset - symbol->size, symbol->size);
                    } else { /*BSS_SECTION*/
                        symbol_val = read_int(memory_buffer + start_bss + symbol->offset, symbol->size);
                    }
                }
                instr.load_store_op.imm = symbol_val;
                break;

            case OP_CALL:
                ASSERT_AND_EXIT(symbol->type != SYMBOL_TYPE_FUNCTION, "ERROR: Symbol is not function type.\n");
                instr.call_op.imm = start_text + symbol->offset - symbol->size;
                break;

            case OP_MOV:
            //case OP_SHR:
            //case OP_SHL:
                ASSERT_AND_EXIT(symbol->type != SYMBOL_TYPE_DATA, "ERROR: Symbol is not data type.\n");
                if (symbol->section == SYMBOL_SECTION_DATA) {
                    symbol_val = read_int(memory_buffer + start_data + symbol->offset - symbol->size, symbol->size);
                } else { /*BSS_SECTION*/
                    symbol_val = read_int(memory_buffer + start_bss + symbol->offset, symbol->size);
                }
                instr.mov_op.imm = (uint32_t)symbol_val;
                break;

            default:
                ASSERT_AND_EXIT(1, "INVALID CODE\n");
                break;
        }

        /* write back to memory */
        memcpy(memory_buffer + start_text + entry->offset, &instr, sizeof(union instruction));
    }

    /* write to ivt position 0 */
    memcpy(memory_buffer + start_ivt, &memory_location, sizeof(union instruction));

    /* refresh entry point */
    registers[AS_REGISTER_LR] = registers[AS_REGISTER_PC];

    /* read from ivt table */
    registers[AS_REGISTER_PC] = memory_location;

    /* add interrupt routines */
    /* first is set processor flags */
    for (int i = 0; i < 4; i += 1) {
        memcpy(memory_buffer + memory_location, &interrupt_routine_set_processor[i], sizeof(union instruction));
        memory_location += sizeof(union instruction);
    }

    registers[AS_REGISTER_SP] = memory_location;

    union instruction instr;
    int32_t evaluated_logic = NONE;
    int64_t evaluated = 0;

    /* timers */
    const uint32_t time_interrupt_const = 1;
    uint32_t current_time = (uint32_t)time(NULL);
    psw_t psw = get_psw(&registers[AS_REGISTER_PSW]);

    /* EXECUTE code */
    while (1) {
        memcpy(&instr, memory_buffer + registers[AS_REGISTER_PC], sizeof(union instruction));
        registers[AS_REGISTER_PC] += sizeof(union instruction);

        if ((uint32_t)time(NULL) - time_interrupt_const > current_time) {
            // generate interrupt and update time
            current_time = (uint32_t) time(NULL);
            uint32_t interrupt_address;
            memcpy(&interrupt_address, memory_buffer + start_ivt + sizeof(uint32_t) * 1,
                   sizeof(uint32_t));

            if ((registers[AS_REGISTER_PSW] & 0x40000000) > 0) {
                if (interrupt_address == 0) {
                    // simulate
                    printf("%d\n", registers[1]++);
                } else {
                    // if we have interrupt routine
                    // put PSW on stack
                    memcpy(memory_buffer + registers[AS_REGISTER_SP], &registers[AS_REGISTER_PSW], sizeof(int32_t));
                    registers[AS_REGISTER_SP] += 4;
                    registers[AS_REGISTER_LR] = registers[AS_REGISTER_PC];
                    registers[AS_REGISTER_PC] = interrupt_address;
                }
            }
        }

        if (instr.instruction.opcode == end_instruction.instruction.opcode
            && instr.instruction.operands == end_instruction.instruction.operands) {
            return 1;
        }

        /* block if we don't have condition */
        if (instr.instruction.condition != NONE) {
            if ((instr.instruction.condition == EQ && evaluated_logic != 0)
                || (instr.instruction.condition == NE && evaluated_logic == 0)
                || (instr.instruction.condition == GT && evaluated_logic != 1)
                || (instr.instruction.condition == GE && evaluated_logic == -1)
                || (instr.instruction.condition == GT && evaluated_logic != -1)
                || (instr.instruction.condition == GE && evaluated_logic == 1)) {
                continue;
            }
        }

        switch (instr.instruction.opcode) {
            case OP_INT:
                memcpy(memory_buffer + registers[AS_REGISTER_SP], &registers[AS_REGISTER_PSW], sizeof(uint32_t));
                registers[AS_REGISTER_SP] += 4;
                registers[AS_REGISTER_LR] = registers[AS_REGISTER_PC];
                memcpy(&registers[AS_REGISTER_PC], memory_buffer + start_ivt + registers[instr.int_op.src], sizeof(uint32_t));
                break;

            case OP_ADD:
                evaluated = registers[instr.arithmetic_op_imm.dst];
                evaluated += instr.arithmetic_op_imm.address_type_bit == ADDRESSING_MODE_IMMEDIATE ?
                            instr.arithmetic_op_imm.imm : registers[instr.arithmetic_op_src.src];
                registers[instr.arithmetic_op_imm.dst] = (int32_t)evaluated;
                break;

            case OP_SUB:
                evaluated = registers[instr.arithmetic_op_imm.dst];
                evaluated -= instr.arithmetic_op_imm.address_type_bit == ADDRESSING_MODE_IMMEDIATE ?
                             instr.arithmetic_op_imm.imm : registers[instr.arithmetic_op_src.src];
                registers[instr.arithmetic_op_imm.dst] = (int32_t)evaluated;
                break;

            case OP_MUL:
                evaluated = registers[instr.arithmetic_op_imm.dst];
                evaluated *= instr.arithmetic_op_imm.address_type_bit == ADDRESSING_MODE_IMMEDIATE ?
                             instr.arithmetic_op_imm.imm : registers[instr.arithmetic_op_src.src];
                registers[instr.arithmetic_op_imm.dst] = (int32_t)evaluated;
                break;

            case OP_DIV:
                evaluated = registers[instr.arithmetic_op_imm.dst];
                evaluated /= instr.arithmetic_op_imm.address_type_bit == ADDRESSING_MODE_IMMEDIATE ?
                             instr.arithmetic_op_imm.imm : registers[instr.arithmetic_op_src.src];
                registers[instr.arithmetic_op_imm.dst] = (int32_t)evaluated;
                break;

            case OP_CMP:
                evaluated = instr.arithmetic_op_imm.address_type_bit == ADDRESSING_MODE_IMMEDIATE ?
                            instr.arithmetic_op_imm.imm : registers[instr.arithmetic_op_src.src];

                if (registers[instr.arithmetic_op_imm.dst] == evaluated) {
                    evaluated_logic = 0;
                } else if (registers[instr.arithmetic_op_imm.dst] >= evaluated) {
                    evaluated_logic = 1;
                } else if (registers[instr.arithmetic_op_imm.dst] <= evaluated) {
                    evaluated_logic = -1;
                }
                break;

            case OP_AND:
                evaluated = registers[instr.logical_op.dst] & registers[instr.logical_op.src];
                registers[instr.logical_op.dst] = evaluated;
                break;

            case OP_OR:
                evaluated = registers[instr.logical_op.dst] | registers[instr.logical_op.src];
                registers[instr.logical_op.dst] = evaluated;
                break;

            case OP_NOT:
                evaluated = ~registers[instr.logical_op.src];
                registers[instr.logical_op.dst] = evaluated;
                break;

            case OP_TEST:
                evaluated = registers[instr.logical_op.dst] & registers[instr.logical_op.src];
                break;

            case OP_IN:
            //case OP_OUT:
                if(instr.io_op.io == IO_BIT_INPUT) {
                    if (registers[instr.io_op.src] == 0x1000) {
                        //if (!kbhit()) {
                            //registers[AS_REGISTER_PC] -= INSTRUCTION_SIZE;
                            //continue;
                        //}
                        io_memory[registers[instr.io_op.src]] = getchar();
                    }
                    registers[instr.io_op.dst] = io_memory[registers[instr.io_op.src]];
                } else {
                    io_memory[registers[instr.io_op.src]] = registers[instr.io_op.dst];
                    if (registers[instr.io_op.src] == (int32_t)0x2000) {
                        printf("%d\n", io_memory[registers[instr.io_op.src]]);
                    }
                }
                break;

            case OP_LDR:
            //case OP_STR:
                if (instr.load_store_op.f == EXTRA_REG_FUNCTION_PRE_INC) {
                    registers[instr.load_store_op.a] += 4;
                } else if (instr.load_store_op.f == EXTRA_REG_FUNCTION_PRE_DEC) {
                    registers[instr.load_store_op.a] -= 4;
                }

                if (instr.load_store_op.ls == MEMORY_FUNCTION_STORE) {
                    memcpy(memory_buffer + registers[instr.load_store_op.a] + instr.load_store_op.imm,
                            &registers[instr.load_store_op.r], sizeof(int32_t));
                } else {
                    memcpy(&registers[instr.load_store_op.r],
                           memory_buffer + registers[instr.load_store_op.a] + instr.load_store_op.imm, sizeof(int32_t));
                }

                if (instr.load_store_op.f == EXTRA_REG_FUNCTION_POST_INC) {
                    registers[instr.load_store_op.a] += 4;
                } else if (instr.load_store_op.f == EXTRA_REG_FUNCTION_POST_DEC) {
                    registers[instr.load_store_op.a] -= 4;
                }
                break;

            case OP_CALL:
                if (instr.instruction.cf == 0) {
                    memcpy(memory_buffer + registers[AS_REGISTER_SP], &registers[AS_REGISTER_PSW], sizeof(uint32_t));
                    registers[AS_REGISTER_SP] += 4;
                }
                registers[AS_REGISTER_LR] = registers[AS_REGISTER_PC];
                registers[AS_REGISTER_PC] = registers[instr.call_op.dst] + instr.call_op.imm;
                break;

            case OP_MOV:
            //case OP_SHR:
            //case OP_SHL:
                if (instr.mov_op.lr == SHIFT_DIRECTION_LEFT && instr.mov_op.imm != 0) {
                    evaluated = registers[instr.mov_op.src] << instr.mov_op.imm;
                } else if (instr.mov_op.lr == SHIFT_DIRECTION_RIGHT && instr.mov_op.imm != 0) {
                    evaluated = registers[instr.mov_op.src] >> instr.mov_op.imm;
                } else {
                    evaluated = registers[instr.mov_op.src];
                }

                registers[instr.mov_op.dst] = (int32_t)evaluated;

                if (instr.mov_op.dst == AS_REGISTER_PC) {
                    // return psw from stack
                    memcpy(&registers[AS_REGISTER_PSW], memory_buffer + registers[AS_REGISTER_SP], sizeof(int32_t));
                    registers[AS_REGISTER_SP] -= 4;
                }
                break;

            case OP_LDCH:
            //case OP_LDCL:
            {
                int_lh val;
                val.intv = registers[instr.ldlh_op.dst];

                if (instr.ldlh_op.hl == PART_BYTE_HIGHER) {
                    val.ints[1] = instr.ldlh_op.c;
                } else {
                    val.ints[0] = instr.ldlh_op.c;
                }

                registers[instr.ldlh_op.dst] = val.intv;
            }
                break;

            default:
                break;
        }

        if (instr.instruction.cf) {
            switch(instr.instruction.opcode) {
                case OP_ADD:
                case OP_SUB:
                case OP_CMP:
                    psw = get_psw(&registers[AS_REGISTER_PSW]);
                    psw.z = (int32_t)evaluated == 0;
                    psw.o = evaluated > INT32_MAX || evaluated < INT32_MIN;
                    psw.c = evaluated != (int32_t)evaluated;
                    psw.n = evaluated < 0;
                    write_psw_to_reg(&registers[AS_REGISTER_PSW], psw);
                    break;

                case OP_MUL:
                case OP_DIV:
                case OP_AND:
                case OP_OR:
                case OP_NOT:
                case OP_TEST:
                    psw = get_psw(&registers[AS_REGISTER_PSW]);
                    psw.z = (int32_t)evaluated == 0;
                    psw.n = evaluated < 0;
                    write_psw_to_reg(&registers[AS_REGISTER_PSW], psw);
                    break;

                default:
                    break;
            }
        }


    }
}

void load_instructions() {
    /* init instruction */
    end_instruction.instruction.opcode = OP_CALL;
    end_instruction.instruction.cf = 0;
    end_instruction.instruction.condition = NONE;
    end_instruction.call_op.dst = 0;
    end_instruction.call_op.imm = INT16_MAX;

    interrupt_routine_set_processor[0].instruction.opcode = OP_LDCH;
    interrupt_routine_set_processor[0].instruction.cf = 0;
    interrupt_routine_set_processor[0].instruction.condition = NONE;
    interrupt_routine_set_processor[0].ldlh_op.dst = 0;
    interrupt_routine_set_processor[0].ldlh_op.c = 0;
    interrupt_routine_set_processor[0].ldlh_op.hl = PART_BYTE_HIGHER;

    interrupt_routine_set_processor[1].instruction.opcode = OP_LDCH;
    interrupt_routine_set_processor[1].instruction.cf = 0;
    interrupt_routine_set_processor[1].instruction.condition = NONE;
    interrupt_routine_set_processor[1].ldlh_op.dst = 0;
    interrupt_routine_set_processor[1].ldlh_op.c = 0;
    interrupt_routine_set_processor[1].ldlh_op.hl = PART_BYTE_LOWER;

    interrupt_routine_set_processor[2].instruction.opcode = OP_MOV;
    interrupt_routine_set_processor[2].instruction.cf = 0;
    interrupt_routine_set_processor[2].instruction.condition = NONE;
    interrupt_routine_set_processor[2].mov_op.src = 0;
    interrupt_routine_set_processor[2].mov_op.dst = AS_REGISTER_PSW;

    interrupt_routine_set_processor[3].instruction.opcode = OP_MOV;
    interrupt_routine_set_processor[3].instruction.cf = 0;
    interrupt_routine_set_processor[3].instruction.condition = NONE;
    interrupt_routine_set_processor[3].mov_op.src = AS_REGISTER_LR;
    interrupt_routine_set_processor[3].mov_op.dst = AS_REGISTER_PC;

    /* Key press routine */

    /* Write to 0x100*/
    on_key_pressed[0].instruction.opcode = OP_LDCL;
    on_key_pressed[0].instruction.cf = 0;
    on_key_pressed[0].instruction.condition = NONE;
    on_key_pressed[0].ldlh_op.hl = PART_BYTE_LOWER;
    on_key_pressed[0].ldlh_op.dst = 1;
    on_key_pressed[0].ldlh_op.c = 0x1000;

    on_key_pressed[1].instruction.opcode = OP_LDCH;
    on_key_pressed[1].instruction.cf = 0;
    on_key_pressed[1].instruction.condition = NONE;
    on_key_pressed[1].ldlh_op.hl = PART_BYTE_HIGHER;
    on_key_pressed[1].ldlh_op.dst = 1;
    on_key_pressed[1].ldlh_op.c = 0;

    on_key_pressed[2].instruction.opcode = OP_IN;
    on_key_pressed[2].instruction.cf = 0;
    on_key_pressed[2].instruction.condition = NONE;
    on_key_pressed[2].io_op.io = IO_BIT_INPUT;
    on_key_pressed[2].io_op.src = 1;
    on_key_pressed[2].io_op.dst = 0;

    /* write 10th bit to 0x1010 */
    on_key_pressed[3].instruction.opcode = OP_LDCL;
    on_key_pressed[3].instruction.cf = 0;
    on_key_pressed[3].instruction.condition = NONE;
    on_key_pressed[3].ldlh_op.hl = PART_BYTE_LOWER;
    on_key_pressed[3].ldlh_op.dst = 2;
    on_key_pressed[3].ldlh_op.c = 0x1010;

    on_key_pressed[4].instruction.opcode = OP_LDCH;
    on_key_pressed[4].instruction.cf = 0;
    on_key_pressed[4].instruction.condition = NONE;
    on_key_pressed[4].ldlh_op.hl = PART_BYTE_HIGHER;
    on_key_pressed[4].ldlh_op.dst = 2;
    on_key_pressed[4].ldlh_op.c = 0;

    /* set 10th bit of reg #3 to 1 0x0400 */
    on_key_pressed[5].instruction.opcode = OP_LDCL;
    on_key_pressed[5].instruction.cf = 0;
    on_key_pressed[5].instruction.condition = NONE;
    on_key_pressed[5].ldlh_op.hl = PART_BYTE_LOWER;
    on_key_pressed[5].ldlh_op.dst = 3;
    on_key_pressed[5].ldlh_op.c = 0x0400;

    on_key_pressed[6].instruction.opcode = OP_LDCH;
    on_key_pressed[6].instruction.cf = 0;
    on_key_pressed[6].instruction.condition = NONE;
    on_key_pressed[6].ldlh_op.hl = PART_BYTE_HIGHER;
    on_key_pressed[6].ldlh_op.dst = 3;
    on_key_pressed[6].ldlh_op.c = 0;

    on_key_pressed[7].instruction.opcode = OP_OUT;
    on_key_pressed[7].instruction.cf = 0;
    on_key_pressed[7].instruction.condition = NONE;
    on_key_pressed[7].io_op.io = IO_BIT_INPUT;
    on_key_pressed[7].io_op.src = 2;
    on_key_pressed[7].io_op.dst = 3;

    /* Pop stack */
    on_key_pressed[8].instruction.opcode = OP_LDR;
    on_key_pressed[8].instruction.cf = 0;
    on_key_pressed[8].instruction.condition = NONE;
    on_key_pressed[8].load_store_op.a = AS_REGISTER_PC;
}