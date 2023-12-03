// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#ifndef ASM_VM_VM_HPP
#define ASM_VM_VM_HPP

#include "machine_code.hpp"

using inst_t = MachineCodeInfo::InstructionTy;
using reg_t = MachineCodeInfo::RegisterValueTy;
using reg_index_t = MachineCodeInfo::RegisterIndexTy;

struct InstructionDecoder {
    inst_t instruction = 0;
    inst_t offset = 0;

    inst_t get(inst_t bit_count) {
        const auto value = (instruction >> offset) & ((1 << bit_count) - 1);
        offset += bit_count;
        return value;
    }

    OpCode get_opcode() { return (OpCode) get(MachineCodeInfo::OPCODE_BITS); }
    BinaryFunc get_binary_func() { return (BinaryFunc) get(MachineCodeInfo::FUNC_SEL_BITS); }
    reg_index_t get_reg() { return get(MachineCodeInfo::REG_BITS); }
};

class VM {
public:
    VM(const inst_t *code, size_t length);

    [[nodiscard]] bool at_end() const { return m_pc >= m_code_length; }

    [[nodiscard]] reg_t get_reg(reg_index_t reg) const;
    void set_reg(reg_index_t reg, reg_t value);

    void execute();
    void step();

private:
    /// Retrieves the next instruction.
    [[nodiscard]] inst_t fetch() const;
    /// Decodes the instruction and execute it.
    void decode(InstructionDecoder instruction);

    void execute_mov(InstructionDecoder instruction);
    void execute_load(InstructionDecoder instruction);
    void execute_loadi(InstructionDecoder instruction);
    void execute_store(InstructionDecoder instruction);
    void execute_binary_inst(InstructionDecoder instruction);

private:
    std::size_t m_pc = 0;
    reg_t m_regs[MachineCodeInfo::REG_COUNT] = {0};
    const inst_t *m_code = nullptr;
    size_t m_code_length = 0;
};

#endif//ASM_VM_VM_HPP
