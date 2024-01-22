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

#define CSI "\x1b["
#define COLOR(style, text) CSI style "m" text CSI "0m"
#define OP(name) COLOR("0", name)
#define REG COLOR("36", "r%d")
#define IMM COLOR("35", "%#x")
#define COMMENT(str) COLOR("32", "; " str)

#define BINOP(opname) OP(opname) " " REG " " REG " " REG "\n"

int cpulm_disassemble_inst(uint32_t inst, uint32_t pc) {
    printf(COLOR("33", "0x%04x \t"), pc);

    const uint32_t opcode = get_bits(inst, 0, 4);

    const uint32_t rd = get_bits(inst, 4, 5);
    const uint32_t rs1 = get_bits(inst, 9, 5);
    const uint32_t rs2 = get_bits(inst, 14, 5);

    switch (opcode) {
    case OP_alu:
        switch (get_bits(inst, 19, 5)) {
        case BF_nor:
            printf(BINOP("nor"), rd, rs1, rs2);
            break;
        case BF_xor:
            printf(BINOP("xor"), rd, rs1, rs2);
            break;
        case BF_add:
            printf(BINOP("add"), rd, rs1, rs2);
            break;
        case BF_sub:
            printf(BINOP("sub"), rd, rs1, rs2);
            break;
        case BF_mul:
            printf(BINOP("mul"), rd, rs1, rs2);
            break;
        case BF_div:
            printf(BINOP("div"), rd, rs1, rs2);
            break;
        case BF_and:
            printf(BINOP("and"), rd, rs1, rs2);
            break;
        case BF_or:
            printf(BINOP("or"), rd, rs1, rs2);
            break;
        default:
            printf(COMMENT("invalid ALU instruction, alu code = %#x") "\n", get_bits(inst, 19, 5));
            return 1;
        }
        break;
    case OP_lsl:
        printf(BINOP("lsl"), rd, rs1, rs2);
        break;
    case OP_asr:
        printf(BINOP("asr"), rd, rs1, rs2);
        break;
    case OP_lsr:
        printf(BINOP("lsr"), rd, rs1, rs2);
        break;
    case OP_load:
        printf(OP("load") " " REG " " REG "\n", rd, rs1);
        break;
    case OP_loadi:
        if (get_bits(inst, 30, 1)) {
            printf(OP("loadi.l") " " REG " " REG " " IMM "\n", rd, rs1, get_bits(inst, 14, 16));
        } else {
            printf(OP("loadi.h") " " REG " " REG " " IMM "\n", rd, rs1, get_bits(inst, 14, 16));
        }
        break;
    case OP_store:
        printf(OP("store") " " REG " " REG "\n", rd, rs1);
        break;
    case OP_jmp:
        printf(OP("jmp") " " REG "\n", rd);
        break;
    case OP_jmpc: {
        const char* flags = compute_flags_string(get_bits(inst, 9, 4));
        printf(OP("jmp.%s") " " REG "\n", flags, rd);
    } break;
    case OP_jmpi:
        printf(OP("jmp") " " IMM "\n", sign_extend_24(get_bits(inst, 4, 24)));
        break;
    case OP_jmpic: {
        const char* flags = compute_flags_string(get_bits(inst, 28, 4));
        printf(OP("jmp.%s") " " IMM "\n", flags, sign_extend_24(get_bits(inst, 4, 24)));
    } break;
    default:
        printf(COMMENT("invalid instruction, opcode = %#x") "\n", opcode);
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
            error_code |= cpulm_disassemble_inst(buffer[i], (uint32_t)i);
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
