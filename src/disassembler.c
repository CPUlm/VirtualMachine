#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

enum opcode_t {
#define INSTRUCTION(name, opcode) OP_##name = opcode,
#include "instructions.def"
};

enum alucode_t {
#define BINARY_INSTRUCTION(name, func) BF_##name,
#include "instructions.def"
};

enum flag_t {
    FLAG_ZERO = 0,
    FLAG_NEGATIVE = 1,
    FLAG_CARRY = 2,
    FLAG_OVERFLOW = 3
};

static inline uint32_t get_bits(uint32_t inst, uint32_t start, uint32_t length) {
    return (inst >> start) & ((1 << length) - 1);
}

static inline const char* compute_flags_string(uint32_t flags) {
    static char buffer[5];
    char* ptr = buffer;

    printf("%d\n", flags);
    if ((flags & (1 << FLAG_ZERO)) != 0)
        *ptr++ = 'z';
    if ((flags & (1 << FLAG_NEGATIVE)) != 0)
        *ptr++ = 'n';
    if ((flags & (1 << FLAG_CARRY)) != 0)
        *ptr++ = 'c';
    if ((flags & (1 << FLAG_OVERFLOW)) != 0)
        *ptr++ = 'v';
    *ptr++ = '\0';
    return &buffer[0];
}

int cpulm_disassemble_inst(uint32_t inst) {
    const uint32_t opcode = get_bits(inst, 0, 4);

    const uint32_t rd = get_bits(inst, 4, 5);
    const uint32_t rs1 = get_bits(inst, 9, 5);
    const uint32_t rs2 = get_bits(inst, 14, 5);

    switch (opcode) {
    case OP_alu:
        switch (get_bits(inst, 19, 5)) {
        case BF_nor:
            printf("nor r%d r%d r%d\n", rd, rs1, rs2);
            break;
        case BF_xor:
            printf("xor r%d r%d r%d\n", rd, rs1, rs2);
            break;
        case BF_add:
            printf("add r%d r%d r%d\n", rd, rs1, rs2);
            break;
        case BF_sub:
            printf("add r%d r%d r%d\n", rd, rs1, rs2);
            break;
        case BF_mul:
            printf("mul r%d r%d r%d\n", rd, rs1, rs2);
            break;
        case BF_div:
            printf("div r%d r%d r%d\n", rd, rs1, rs2);
            break;
        case BF_and:
            printf("and r%d r%d r%d\n", rd, rs1, rs2);
            break;
        case BF_or:
            printf("or r%d r%d r%d\n", rd, rs1, rs2);
            break;
        default:
            printf("; invalid ALU instruction, alu code = %#x\n", get_bits(inst, 19, 5));
            return 1;
        }
        break;
    case OP_lsl:
        printf("lsl r%d r%d r%d\n", rd, rs1, rs2);
        break;
    case OP_asr:
        printf("asr r%d r%d r%d\n", rd, rs1, rs2);
        break;
    case OP_lsr:
        printf("lsr r%d r%d r%d\n", rd, rs1, rs2);
        break;
    case OP_load:
        printf("load r%d r%d\n", rd, rs1);
        break;
    case OP_loadi:
        if (get_bits(inst, 30, 1)) {
            printf("loadi.l r%d r%d %#x\n", rd, rs1, get_bits(inst, 14, 16));
        } else {
            printf("loadi.h r%d r%d %#x\n", rd, rs1, get_bits(inst, 14, 16));
        }
        break;
    case OP_store:
        printf("store r%d r%d\n", rd, rs1);
        break;
    case OP_jmp:
        printf("jmp r%d\n", rd);
        break;
    case OP_jmpc: {
        const char* flags = compute_flags_string(get_bits(inst, 9, 4));
        printf("jmpc.%s r%d\n", flags, rd);
    } break;
    case OP_jmpi:
        printf("jmpi %d\n", sign_extend_24(get_bits(inst, 4, 24)));
        break;
    case OP_jmpic: {
        const char* flags = compute_flags_string(get_bits(inst, 28, 4));
        printf("jmpic.%s %d\n", flags, sign_extend_24(get_bits(inst, 4, 24)));
    } break;
    default:
        printf("; invalid instruction, opcode = %#x\n", opcode);
        return 1;
    }

    return 0;
}

int cpulm_disassemble_file(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL)
        return -1;

    uint32_t buffer[1024];
    size_t read_words = 0;
    int error_code = 0;
    while ((read_words = fread(buffer, sizeof(uint32_t), 1024, file)) > 0) {
        for (size_t i = 0; i < read_words; i++)
            error_code |= cpulm_disassemble_inst(buffer[i]);
    }

    fclose(file);
    return error_code;
}

#ifdef DISASSEMBLER_AS_PROGRAM
int main(int argc, char* argv[]) {
    if (argc <= 1) {
        fprintf(stderr, "\x1b[31merror:\x1b[0m missing program binary file\n");
        fprintf(stderr, "USAGE: %s file_path\n", argv[0]);
        return EXIT_FAILURE;
    }

    int error_code = cpulm_disassemble_file(argv[1]);
    if (error_code == -1) {
        fprintf(stderr, "\x1b[31merror:\x1b[0m failed to open file '%s'\n", argv[1]);
        return EXIT_FAILURE;
    }

    return error_code;
}
#endif
