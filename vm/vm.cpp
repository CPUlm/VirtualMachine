// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#include "vm.hpp"

#include <cstdio>
#include <cstdlib>

VM::VM(const inst_t *code, size_t length)
    : m_code(code), m_code_length(length) {
}

void VM::execute() {
    while (!at_end())
        step();
}

void VM::step() {
    InstructionDecoder decoder;
    decoder.instruction = fetch();
    m_pc += 1;
    decode(decoder);
}

[[nodiscard]] inst_t VM::fetch() const {
    return m_code[m_pc];
}

void VM::decode(InstructionDecoder instruction) {
    const OpCode opcode = instruction.get_opcode();
    switch (opcode) {
        case OP_binary_inst:
            return execute_binary_inst(instruction);
        case OP_mov:
            return execute_mov(instruction);
        case OP_load:
            return execute_load(instruction);
        case OP_loadi:
            return execute_loadi(instruction);
        case OP_store:
            return execute_store(instruction);
        default:
            fprintf(stderr, "ERROR: machine code ill-formed\n");
            exit(EXIT_FAILURE);
            return;
    }
}

void VM::execute_mov(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs = instruction.get_reg();
    m_regs[rd] = m_regs[rs];
}

void VM::execute_load(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs = instruction.get_reg();
    // TODO
}

void VM::execute_loadi(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    uint16_t imm = instruction.get(16);
    bool lhw = instruction.get(1);

    if (lhw) {
        m_regs[rd] = imm;
    } else {
        m_regs[rd] = imm << 16;
    }
}

void VM::execute_store(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs = instruction.get_reg();
    // TODO
}

void VM::execute_binary_inst(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs1 = instruction.get_reg();
    reg_index_t rs2 = instruction.get_reg();
    BinaryFunc func = instruction.get_binary_func();

    const reg_t rs1_val = get_reg(rs1);
    const reg_t rs2_val = get_reg(rs2);

    reg_t rd_val = 0;
    switch (func) {
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
        case BF_and:
            rd_val = rs1_val & rs2_val;
            break;
        case BF_or:
            rd_val = rs1_val | rs2_val;
            break;
        case BF_xor:
            rd_val = rs1_val ^ rs2_val;
            break;
        case BF_nor:
            rd_val = ~(rs1_val | rs2_val);
            break;
    }

    set_reg(rd, rd_val);
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
