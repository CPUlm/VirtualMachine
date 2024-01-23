// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#include "vm.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "screen.h"
#include "utils.h"

void Breakpoint::enable(inst_t* code) {
    old_inst = code[addr];
    code[addr] = OP_break;
    is_enabled = true;
}

void Breakpoint::disable(inst_t* code) {
    code[addr] = old_inst;
    is_enabled = false;
}

static void synchronize_time(ram_t* ram, addr_t addr, word_t word) {
    if (word > 0) {
        time_t now = time(0);
        tm* tm = localtime(&now);

        ram_set(ram, 1025, 0);
        ram_set(ram, 1026, 1);
        ram_set(ram, 1027, tm->tm_sec % 60 /* because of the leap second */);
        ram_set(ram, 1028, tm->tm_min);
        ram_set(ram, 1029, tm->tm_hour);
        ram_set(ram, 1029, tm->tm_mday);
        ram_set(ram, 1031, tm->tm_mon);
        ram_set(ram, 1032, tm->tm_year + 1900);
        ram_set(ram, 1033, (tm->tm_wday + 6) % 7);
    }
}

VM::VM(std::vector<std::uint32_t>& rom_data, const std::vector<std::uint32_t>& ram_data, bool use_screen, const char* code_filename)
    : m_code_filename(code_filename)
    , m_code(rom_data.data())
    , m_code_length(rom_data.size())
    , m_ram(ram_create())
    , m_use_screen(use_screen) {
    if (m_use_screen)
        screen_init_with_ram_mapping(m_ram);
    ram_init(m_ram, ram_data.data(), ram_data.size());
    ram_install_write_listener(m_ram, 1025, 1025, &synchronize_time);
    m_previous_cycle_time = std::chrono::steady_clock::now();
}

VM::~VM() {
    ram_destroy(m_ram);
    if (m_use_screen)
        screen_terminate();
}

void VM::execute() {
    while (!at_end() && !m_at_breakpoint) {
        step();
    }

    m_at_breakpoint = false;
}

void VM::step() {
    if (at_end())
        return;

    auto now = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::nanoseconds>(now - m_previous_cycle_time).count();
    if (dur >= 1'000'000'000) {
        // 1 second has elapsed
        ram_set(m_ram, 1024, 1);
        m_previous_cycle_time = now;
    }

    InstructionDecoder decoder;
    if (m_pc >= m_code_length) {
        VM::error("jumping outside of program.");
    }

    decoder.instruction = m_code[m_pc];
    m_pc++;
    execute(decoder);
}

bool VM::test_flags(size_t select) {
    for (int i = 0; i < MachineCodeInfo::NB_FLAGS; i++)
        if (((select & (1 << i)) != 0) && m_flags[i])
            return true;
    return false;
}

void VM::execute(InstructionDecoder instruction) {
    const opcode_t opcode = instruction.get_opcode();
    switch (opcode) {
    case OP_alu:
        return execute_alu(instruction);
    case OP_lsl:
        return execute_lsl(instruction);
    case OP_asr:
        return execute_asr(instruction);
    case OP_lsr:
        return execute_lsr(instruction);
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
    case OP_break:
        return execute_break(instruction);
    default:
        error("invalid opcode");
        return;
    }
}

void VM::execute_alu(InstructionDecoder instruction) {
    // TODO : update the flags
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs1 = instruction.get_reg();
    reg_index_t rs2 = instruction.get_reg();
    alucode_t alucode = instruction.get_alucode();

    const reg_t rs1_val = get_reg(rs1);
    const reg_t rs2_val = get_reg(rs2);

    std::memset(m_flags, 0, sizeof(m_flags));

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
        m_flags[FLAG_OVERFLOW] = __builtin_add_overflow((std::int32_t)rs1_val, (std::int32_t)rs2_val, (std::int32_t*)&rd_val);
        m_flags[FLAG_CARRY] = __builtin_add_overflow((std::uint32_t)rs1_val, (std::uint32_t)rs2_val, (std::uint32_t*)&rd_val);
        break;
    case BF_sub:
        m_flags[FLAG_OVERFLOW] = __builtin_sub_overflow((std::int32_t)rs1_val, (std::int32_t)rs2_val, (std::int32_t*)&rd_val);
        m_flags[FLAG_CARRY] = __builtin_sub_overflow((std::uint32_t)rs1_val, (std::uint32_t)rs2_val, (std::uint32_t*)&rd_val);
        break;
    case BF_mul:
        m_flags[FLAG_OVERFLOW] = __builtin_mul_overflow((std::int32_t)rs1_val, (std::int32_t)rs2_val, (std::int32_t*)&rd_val);
        m_flags[FLAG_CARRY] = __builtin_mul_overflow((std::uint32_t)rs1_val, (std::uint32_t)rs2_val, (std::uint32_t*)&rd_val);
        break;
    case BF_div:
        rd_val = rs1_val / rs2_val;
        break;
    default:
        error("invalid ALU code");
        break;
    }

    m_flags[FLAG_ZERO] = (rd_val == 0);
    m_flags[FLAG_NEGATIVE] = (((std::int32_t)rd_val) < 0);

    set_reg(rd, rd_val);
}

void VM::execute_lsl(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs1 = instruction.get_reg();
    reg_index_t rs2 = instruction.get_reg();

    const reg_t rs1_val = get_reg(rs1);
    const reg_t rs2_val = get_reg(rs2) & 0b11111;
    set_reg(rd, rs1_val << rs2_val);
}

void VM::execute_asr(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs1 = instruction.get_reg();
    reg_index_t rs2 = instruction.get_reg();

    const reg_t rs1_val = get_reg(rs1);
    const reg_t rs2_val = get_reg(rs2) & 0b11111;
    // Logical shift is done when operating on SIGNED values.
    set_reg(rd, (std::int32_t)(rs1_val) >> rs2_val);
}

void VM::execute_lsr(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs1 = instruction.get_reg();
    reg_index_t rs2 = instruction.get_reg();

    const reg_t rs1_val = get_reg(rs1);
    const reg_t rs2_val = get_reg(rs2) & 0b11111;
    // Logical shift is done when operating on UNSIGNED values.
    set_reg(rd, (std::uint32_t)(rs1_val) >> rs2_val);
}

void VM::execute_load(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs = instruction.get_reg();
    set_reg(rd, ram_get(m_ram, get_reg(rs)));
}

void VM::execute_loadi(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs = instruction.get_reg();
    uint16_t imm = instruction.get(16);
    bool lhw = instruction.get(1);

    if (lhw) {
        set_reg(rd, get_reg(rs) + imm);
    } else {
        set_reg(rd, get_reg(rs) + (imm << 16));
    }
}

void VM::execute_store(InstructionDecoder instruction) {
    reg_index_t rd = instruction.get_reg();
    reg_index_t rs = instruction.get_reg();
    ram_set(m_ram, get_reg(rd), get_reg(rs));
}

void VM::execute_jmp(InstructionDecoder instruction) {
    reg_index_t rs = instruction.get_reg();
    m_pc = get_reg(rs);
}

void VM::execute_jmpi(InstructionDecoder instruction) {
    int32_t imm = sign_extend_24(instruction.get(24));
    m_pc += imm - 1;
}

void VM::execute_jmpc(InstructionDecoder instruction) {
    reg_index_t rs = instruction.get_reg();
    size_t select = instruction.get(MachineCodeInfo::NB_FLAGS);
    if (test_flags(select))
        m_pc = get_reg(rs);
}

void VM::execute_jmpic(InstructionDecoder instruction) {
    int32_t imm = sign_extend_24(instruction.get(24));
    size_t select = instruction.get(MachineCodeInfo::NB_FLAGS);
    if (test_flags(select))
        m_pc += imm - 1;
}

void VM::execute_break(InstructionDecoder) {
    m_pc -= 1;
    printf("Breakpoint at PC = %#lx (%lu) reached.\n", m_pc, m_pc);
    m_breakpoints[m_pc].disable(m_code);
    m_at_breakpoint = true;
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

bool VM::at_end() const {
    return m_pc == 0xffffffff;
}

void VM::add_breakpoint(addr_t addr) {
    auto it = m_breakpoints.find(addr);
    if (it == m_breakpoints.end()) {
        Breakpoint breakpoint;
        breakpoint.addr = addr;
        breakpoint.enable(m_code);
        m_breakpoints.insert({ addr, breakpoint });
        printf("Breakpoint added at %#x\n", addr);
    } else {
        it->second.enable(m_code);
        printf("Breakpoint enabled at %#x\n", addr);
    }
}

void VM::remove_breakpoint(addr_t pc) {
    auto it = m_breakpoints.find(pc);
    if (it == m_breakpoints.end())
        return;

    it->second.disable(m_code);
    m_breakpoints.erase(it);
}

void VM::print_breakpoints() {
    if (m_breakpoints.empty()) {
        printf("No breakpoints\n");
        return;
    }

    printf("There is %lu breakpoint(s):\n", m_breakpoints.size());
    for (auto& [addr, breakpoint] : m_breakpoints) {
        printf("  - Breakpoint at %#x", addr);
        if (!breakpoint.is_enabled)
            printf(" (disabled)");
        printf("\n");
    }
}
