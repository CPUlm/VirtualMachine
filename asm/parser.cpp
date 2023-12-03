// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#include "parser.hpp"

#include <cassert>
#include <cstdlib>
#include <cstdio>

Parser::Parser(Lexer &lexer)
    : m_lexer(lexer) {

#define REG(name, i) m_registers.insert({name, i});
#include "regs.def"

#define INSTRUCTION(name, opcode) m_opcodes.insert({#name, INST_##name});
#define PSEUDO_INSTRUCTION(name) m_opcodes.insert({#name, INST_##name});
#include "instructions.def"
}

std::vector<uint32_t> Parser::parse() {
    m_output.clear();

    Token token = m_lexer.next();
    while (token != Token::END_OF_FILE) {
        if (token == Token::DIRECTIVE) {
            parse_directive();
        } else if (token == Token::IDENTIFIER) {
            parse_identifier();
        }

        token = m_lexer.next();
    }

    return m_output;
}

uint8_t Parser::parse_register() {
    if (m_lexer.next() != Token::IDENTIFIER)
        expect("a register");

    auto register_spelling = m_lexer.get_last_token_spelling();
    auto it = m_registers.find(register_spelling);
    if (it == m_registers.end())
        expect("a register");

    return it->second & MachineCodeInfo::REG_MASK;
}

intmax_t Parser::parse_immediate() {
    intmax_t sign = 1;

    Token token = m_lexer.next();
    while (token == Token::MINUS) {
        sign *= -1;
        token = m_lexer.next();
    }

    if (token != Token::IMMEDIATE)
        expect("an immediate");

    return sign * m_lexer.get_last_parsed_immediate();
}

void Parser::parse_directive() {
}

void Parser::parse_identifier() {
    auto opcode_spelling = m_lexer.get_last_token_spelling();
    auto it = m_opcodes.find(opcode_spelling);
    if (it == m_opcodes.end())
        expect("an instruction");

    parse_instruction(it->second);
}

void Parser::parse_instruction(Instruction opcode) {
    switch (opcode) {
        case INST_mov:
            return parse_mov_inst();
        case INST_load:
            return parse_load_inst();
        case INST_loadi:
            return parse_loadi_inst();
        case INST_store:
            return parse_store_inst();
        case INST_push:
            return parse_push_inst();
        case INST_pop:
            return parse_pop_inst();
        case INST_add:
            return parse_binary_inst(BF_add);
        case INST_sub:
            return parse_binary_inst(BF_sub);
        case INST_mul:
            return parse_binary_inst(BF_mul);
        case INST_div:
            return parse_binary_inst(BF_div);
        case INST_and:
            return parse_binary_inst(BF_and);
        case INST_or:
            return parse_binary_inst(BF_or);
        case INST_xor:
            return parse_binary_inst(BF_xor);
        case INST_nor:
            return parse_binary_inst(BF_nor);
        case INST_not:
            return parse_not_inst();
        case INST_neg:
            return parse_neg_inst();
        default:
            assert(false && "unreachable");
    }
}

void Parser::parse_mov_inst() {
    const auto rd = parse_register();
    const auto rs = parse_register();

    uint32_t instruction = OpCode::OP_mov;
    instruction |= rd << MachineCodeInfo::OPCODE_BITS;
    instruction |= rs << (MachineCodeInfo::OPCODE_BITS + MachineCodeInfo::REG_BITS);
    push_instruction(instruction);
}

void Parser::parse_load_inst() {
    const auto rd = parse_register();
    const auto rs = parse_register();

    uint32_t instruction = OpCode::OP_load;
    instruction |= rd << MachineCodeInfo::OPCODE_BITS;
    instruction |= rs << (MachineCodeInfo::OPCODE_BITS + MachineCodeInfo::REG_BITS);
    push_instruction(instruction);
}

void Parser::parse_loadi_inst(bool lhw) {
    const auto rd = parse_register();
    const auto imm = parse_immediate() & ((1 << 16) - 1);

    uint32_t instruction = OpCode::OP_loadi;
    instruction |= rd << MachineCodeInfo::OPCODE_BITS;
    instruction |= imm << (MachineCodeInfo::OPCODE_BITS + MachineCodeInfo::REG_BITS);
    instruction |= lhw << (MachineCodeInfo::OPCODE_BITS + MachineCodeInfo::REG_BITS + 16);
    push_instruction(instruction);
}

void Parser::parse_store_inst() {
    const auto rd = parse_register();
    const auto rs = parse_register();

    uint32_t instruction = OpCode::OP_store;
    instruction |= rd << MachineCodeInfo::OPCODE_BITS;
    instruction |= rs << (MachineCodeInfo::OPCODE_BITS + MachineCodeInfo::REG_BITS);
    push_instruction(instruction);
}

void Parser::parse_push_inst() {
    const auto rs = parse_register();

    uint32_t instruction = OpCode::OP_push;
    instruction |= rs << MachineCodeInfo::OPCODE_BITS;
    push_instruction(instruction);
}

void Parser::parse_pop_inst() {
    const auto rd = parse_register();

    uint32_t instruction = OpCode::OP_pop;
    instruction |= rd << MachineCodeInfo::OPCODE_BITS;
    push_instruction(instruction);
}

void Parser::parse_binary_inst(BinaryFunc func) {
    const auto rd = parse_register();
    const auto rs1 = parse_register();
    const auto rs2 = parse_register();

    uint32_t instruction = OpCode::OP_binary_inst;
    instruction |= rd << MachineCodeInfo::OPCODE_BITS;
    instruction |= rs1 << (MachineCodeInfo::OPCODE_BITS + MachineCodeInfo::REG_BITS);
    instruction |= rs2 << (MachineCodeInfo::OPCODE_BITS + 2 * MachineCodeInfo::REG_BITS);
    instruction |= func << (MachineCodeInfo::OPCODE_BITS + 3 * MachineCodeInfo::REG_BITS);
    push_instruction(instruction);
}

void Parser::parse_not_inst() {
    const auto rd = parse_register();
    const auto rs = parse_register();

    // NOR rd rs rs
    uint32_t instruction = OpCode::OP_binary_inst;
    instruction |= rd << MachineCodeInfo::OPCODE_BITS;
    instruction |= rs << (MachineCodeInfo::OPCODE_BITS + MachineCodeInfo::REG_BITS);
    instruction |= rs << (MachineCodeInfo::OPCODE_BITS + 2 * MachineCodeInfo::REG_BITS);
    instruction |= BF_nor << (MachineCodeInfo::OPCODE_BITS + 3 * MachineCodeInfo::REG_BITS);
    push_instruction(instruction);
}

void Parser::parse_neg_inst() {
    const auto rd = parse_register();
    const auto rs = parse_register();

    // SUB rd $0 rs
    uint32_t instruction = OpCode::OP_binary_inst;
    instruction |= rd << MachineCodeInfo::OPCODE_BITS;
    instruction |= 0 << (MachineCodeInfo::OPCODE_BITS + MachineCodeInfo::REG_BITS);
    instruction |= rs << (MachineCodeInfo::OPCODE_BITS + 2 * MachineCodeInfo::REG_BITS);
    instruction |= BF_sub << (MachineCodeInfo::OPCODE_BITS + 3 * MachineCodeInfo::REG_BITS);
    push_instruction(instruction);
}

void Parser::expect(const char *token_name) {
    fprintf(stderr, "ERROR: expected %s\n", token_name);
    std::exit(EXIT_FAILURE);
}

void Parser::push_instruction(uint32_t instruction) {
    m_output.push_back(instruction);
}
