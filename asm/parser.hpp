// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#ifndef ASM_ASM_PARSER_HPP
#define ASM_ASM_PARSER_HPP

#include "lexer.hpp"
#include "machine_code.hpp"

#include <string_view>
#include <unordered_map>
#include <vector>

class Parser {
public:
    explicit Parser(Lexer &lexer);

    [[nodiscard]] std::vector<uint32_t> parse();

private:
    uint8_t parse_register();
    intmax_t parse_immediate();
    void parse_directive();
    void parse_identifier();
    void parse_instruction(Instruction opcode);
    void parse_mov_inst();
    void parse_load_inst();
    void parse_loadi_inst(bool lhw = true);
    void parse_store_inst();
    void parse_push_inst();
    void parse_pop_inst();
    void parse_binary_inst(BinaryFunc func);
    void parse_not_inst();
    void parse_neg_inst();

    [[noreturn]] void expect(const char *token_name);

    void push_instruction(uint32_t instruction);

private:
    Lexer &m_lexer;
    std::unordered_map<std::string_view, uint8_t> m_registers;
    std::unordered_map<std::string_view, Instruction> m_opcodes;
    std::vector<uint32_t> m_output;
};

#endif//ASM_ASM_PARSER_HPP
