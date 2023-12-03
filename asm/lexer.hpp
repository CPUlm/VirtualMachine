// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#ifndef ASM_ASM_LEXER_HPP
#define ASM_ASM_LEXER_HPP

#include "token.hpp"

#include <cstdint>
#include <string_view>

class Lexer {
public:
    explicit Lexer(const char *input);

    Token next();

    [[nodiscard]] std::intmax_t get_last_parsed_immediate() const { return m_last_parsed_immediate; }
    [[nodiscard]] std::string_view get_last_token_spelling() const { return std::string_view(m_token_begin, std::distance(m_token_begin, m_token_end)); }

private:
    void skip_whitespace();
    void skip_comment();
    Token lex_bin_immediate();
    Token lex_hex_immediate();
    Token lex_dec_immediate();
    Token lex_directive();
    Token lex_identifier();

private:
    const char *m_input = nullptr;
    const char *m_token_begin = nullptr;
    const char *m_token_end = nullptr;
    std::intmax_t m_last_parsed_immediate = 0;
};

#endif//ASM_ASM_LEXER_HPP
