// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#include "vm.hpp"

#include <cstdio>
#include <cstdlib>

VM::VM(const inst_t* code, size_t length)
    : m_code(code)
    , m_code_length(length) {
}

void VM::execute() {
    while (!at_end())
        step();
}

void VM::step() {
    InstructionDecoder decoder;
    decoder.instruction = m_code[m_pc];
    m_pc += 1;
    execute(decoder);
}

ram_word_t VM::read_ram(ram_index_t adr) {
    if (ram.size() <= adr) {
        ram.resize(adr + 1, 0);
        fprintf(stderr,
            "WARNING : try to read the ram without initialisation\n");
    }
    return ram[adr];
}

void VM::write_ram(ram_index_t adr, ram_word_t value) {
    if (ram.size() <= adr) {
        ram.resize(adr + 1, 0);
    }
    ram[adr] = value;
}

bool VM::test_flags(size_t select) {
    for (int i = 0; i < MachineCodeInfo::NB_FLAGS; i++)
        if ((select & (1 << i)) && flags[i])
            return true;
    return false;
}

void VM::execute(InstructionDecoder instruction) {
    const opcode_t opcode = instruction.get_opcode();
    switch (opcode) {
    case OP_alu:
        return execute_binary_inst(instruction);
    case OP_load:
        return execute_load(instruction);
    case OP_loadi:
        return execute_loadi(instruction);
    case OP_store:
        return execute_store(instruction);
    case OP_jmp:
        return execute_jmp(instruction);
    case OP_jmpi:
        return execute_jmpi(instruction);
    case OP_jmpc:
        return execute_jmpc(instruction);
    case OP_jmpic:
        return execute_jmpic(instruction);
    default:
        error("invalid opcode");
        return;
    }
}

void VM::execute_load(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs = instruction.get_reg();
    m_regs[rd] = read_ram(get_reg(rs));
}

void VM::execute_loadi(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs = instruction.get_reg();
    uint16_t imm = instruction.get(16);
    bool lhw = instruction.get(1);

    if (lhw) {
        m_regs[rd] = get_reg(rs) + imm;
    } else {
        m_regs[rd] = get_reg(rs) + (imm << 16);
    }
}

void VM::execute_store(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs = instruction.get_reg();
    write_ram(get_reg(rd), get_reg(rs));
}

void VM::execute_binary_inst(InstructionDecoder instruction) {
    // TODO : update the flags
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs1 = instruction.get_reg();
    reg_index_t rs2 = instruction.get_reg();
    alucode_t alucode = instruction.get_alucode();

    const reg_t rs1_val = get_reg(rs1);
    const reg_t rs2_val = get_reg(rs2);

    reg_t rd_val = 0;
    switch (alucode) {
    case BF_and:
        rd_val = rs1_val & rs2_val;
        break;
    case BF_or:
        rd_val = rs1_val | rs2_val;
        break;
    case BF_nor:
        rd_val = ~(rs1_val | rs2_val);
        break;
    case BF_xor:
        rd_val = rs1_val ^ rs2_val;
        break;
    case BF_add:
        rd_val = rs1_val + rs2_val;
        break;
    case BF_sub:
        rd_val = rs1_val - rs2_val;
        break;
    case BF_mul:
        rd_val = rs1_val * rs2_val;
        break;
    case BF_div:
        rd_val = rs1_val / rs2_val;
        break;
    default:
        error("invalid ALU code");
        break;
    }

    set_reg(rd, rd_val);
}

void VM::execute_jmp(InstructionDecoder instruction) {
    reg_index_t rs = instruction.get_reg();
    m_pc = get_reg(rs) - 1;
}

void VM::execute_jmpi(InstructionDecoder instruction) {
    uint16_t imm = instruction.get(16);
    m_pc += imm;
}

void VM::execute_jmpc(InstructionDecoder instruction) {
    reg_index_t rs = instruction.get_reg();
    size_t select = instruction.get(MachineCodeInfo::NB_FLAGS);
    if (test_flags(select))
        m_pc = get_reg(rs) - 1;
}

void VM::execute_jmpic(InstructionDecoder instruction) {
    uint16_t imm = instruction.get(16);
    size_t select = instruction.get(MachineCodeInfo::NB_FLAGS);
    if (test_flags(select))
        m_pc += imm;
}

void VM::warning(const char* msg) {
    fprintf(stderr, "\x1b[1;33mWARNING:\x1b[0m %s\n", msg);
}

void VM::error(const char* msg) {
    fprintf(stderr, "\x1b[1;31mERROR:\x1b[0m machine code ill-formed; %s\n",
        msg);
    exit(EXIT_FAILURE);
}

reg_t VM::get_reg(reg_index_t reg) const {
    if (reg <= 1)
        return reg;
    else
        return m_regs[reg];
}

void VM::set_reg(reg_index_t reg, reg_t value) {
    m_regs[reg] = value;
}
