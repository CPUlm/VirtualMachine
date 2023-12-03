// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#ifndef ASM_COMMON_MACHINE_CODE_HPP
#define ASM_COMMON_MACHINE_CODE_HPP

#include <cstddef>
#include <cstdint>
#include <limits>

/*
 * Description of the machine code.
 */

struct MachineCodeInfo {
    /// Type of an encoded instruction.
    using InstructionTy = std::uint32_t;

    /// Count of bits used to encode an instruction.
    static constexpr size_t INSTRUCTION_BITS = std::numeric_limits<InstructionTy>::digits;

    /// Type of an operation code.
    using OpCodeTy = std::uint8_t;

    /// Count of bits used to encode a opcode.
    static constexpr size_t OPCODE_BITS = 6;
    /// Bit mask used to retrieve an opcode.
    static constexpr InstructionTy OPCODE_MASK = (1 << OPCODE_BITS) - 1;
    /// The total maximum count of supported opcodes.
    static constexpr size_t OPCODE_COUNT = (1 << OPCODE_BITS);

    /// Type of a register index/name.
    using RegisterIndexTy = std::uint8_t;
    /// Type of a register value.
    using RegisterValueTy = std::uint32_t;

    /// Count of bits used to encode a register index.
    static constexpr size_t REG_BITS = 5;
    /// Bit mask used to retrieve a register index.
    static constexpr InstructionTy REG_MASK = (1 << REG_BITS) - 1;
    /// The minimum allowed register index (inclusive).
    static constexpr RegisterIndexTy REG_MIN = 0;
    /// The maximum allowed register index (inclusive).
    static constexpr RegisterIndexTy REG_MAX = (1 << REG_BITS) - 1;
    /// The total count of supported registers.
    static constexpr size_t REG_COUNT = REG_MAX - REG_MIN + 1;

    /// Type of the function selector for binary instructions.
    using FuncSelTy = std::uint8_t;
    /// Count of bits used to encode the function selector for binary instructions.
    static constexpr size_t FUNC_SEL_BITS = 5;
    static constexpr size_t FUNC_SEL_MASK = (1 << FUNC_SEL_BITS) - 1;
};

static_assert(MachineCodeInfo::OPCODE_BITS <= MachineCodeInfo::INSTRUCTION_BITS);
static_assert(MachineCodeInfo::REG_BITS <= MachineCodeInfo::INSTRUCTION_BITS);

enum Instruction {
#define INSTRUCTION(name, opcode) INST_##name,
#define PSEUDO_INSTRUCTION(name) INST_##name,
#include "instructions.def"
};

enum OpCode {
    OP_binary_inst = 0,
#define INSTRUCTION(name, opcode) OP_##name = opcode,
#include "instructions.def"
};

enum BinaryFunc {
#define BINARY_INSTRUCTION(name, func) BF_##name,
#include "instructions.def"
};

#endif//ASM_COMMON_MACHINE_CODE_HPP
