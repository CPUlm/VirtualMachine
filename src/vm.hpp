// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#ifndef ASM_VM_VM_HPP
#define ASM_VM_VM_HPP

#include "machine_code.hpp"
#include "memory.h"
#include <chrono>
#include <vector>

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

    opcode_t get_opcode() {
        return (opcode_t)get(MachineCodeInfo::OPCODE_BITS);
    }
    alucode_t get_alucode() {
        return (alucode_t)get(MachineCodeInfo::ALUCODE_BITS);
    }
    reg_index_t get_reg() { return get(MachineCodeInfo::REG_BITS); }
};

class VM {
public:
    VM(const std::vector<std::uint32_t>& rom_data, const std::vector<std::uint32_t>& ram_data, bool use_screen = true);
    ~VM();

    [[nodiscard]] bool at_end() const;

    [[nodiscard]] std::uint32_t get_pc() const { return m_pc; }
    [[nodiscard]] reg_t get_reg(reg_index_t reg) const;
    void set_reg(reg_index_t reg, reg_t value);

    [[nodiscard]] bool get_flag(flag_t flag) const { return m_flags[flag]; }

    void execute();
    void step();

private:
    bool test_flags(size_t select);

    void execute(InstructionDecoder instruction);
    void execute_alu(InstructionDecoder instruction);
    void execute_lsl(InstructionDecoder instruction);
    void execute_asr(InstructionDecoder instruction);
    void execute_lsr(InstructionDecoder instruction);
    void execute_load(InstructionDecoder instruction);
    void execute_loadi(InstructionDecoder instruction);
    void execute_store(InstructionDecoder instruction);
    void execute_jmp(InstructionDecoder instruction);
    void execute_jmpi(InstructionDecoder instruction);
    void execute_jmpc(InstructionDecoder instruction);
    void execute_jmpic(InstructionDecoder instruction);

    static void warning(const char* msg);
    static void error(const char* msg);

private:
    std::chrono::steady_clock::time_point m_previous_cycle_time;
    std::uint64_t m_pc = 0;
    reg_t m_regs[MachineCodeInfo::REG_COUNT] = { 0 };
    const inst_t* m_code = nullptr;
    size_t m_code_length = 0;
    ram_t* m_ram = nullptr;
    bool m_use_screen = false;
    bool m_flags[MachineCodeInfo::NB_FLAGS] = { false };
};

#endif // ASM_VM_VM_HPP
