// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#include "lexer.hpp"

#include <cassert>

Lexer::Lexer(const char *input)
    : m_input(input) {
    assert(input != nullptr);
}

[[nodiscard]] static bool is_whitespace(char ch) {
    switch (ch) {
        case ' ':
        case '\t':
        case '\f':
        case '\v':
        case '\n':
        case '\r':
            return true;
        default:
            return false;
    }
}

[[nodiscard]] static bool is_newline(char ch) {
    return ch == '\n' || ch == '\r';
}

[[nodiscard]] static bool is_bin_digit(char ch) {
    return ch == '0' || ch == '1';
}

[[nodiscard]] static bool is_digit(char ch) {
    return ch >= '0' && ch <= '9';
}

[[nodiscard]] static bool is_hex_digit(char ch) {
    return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

[[nodiscard]] static bool is_ident_start(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

[[nodiscard]] static bool is_ident_cont(char ch) {
    return is_ident_start(ch) || is_digit(ch);
}

Token Lexer::next() {
start:
    skip_whitespace();

    m_token_begin = m_input;

    // EOF -- End-of-File
    if (*m_input == '\0') {
        return Token::END_OF_FILE;
    }

    // Line comment
    if (*m_input == ';') {
        skip_comment();
        // We avoid recursive call to next() because all compilers (and all in Debug mode)
        // do not optimize recursive tail calls.
        goto start;
    }

    // Colon
    if (*m_input == ':') {
        m_input += 1;
        m_token_end = m_input;
        return Token::COLON;
    }

    // Minus
    if (*m_input == '-') {
        m_input += 1;
        m_token_end = m_input;
        return Token::MINUS;
    }

    // Immediate
    if (*m_input == '0') {
        if (m_input[1] == 'b' || m_input[1] == 'B')
            return lex_bin_immediate();
        if (m_input[1] == 'x' || m_input[1] == 'X')
            return lex_hex_immediate();
        return lex_dec_immediate();
    } else if (is_digit(*m_input)) {
        return lex_dec_immediate();
    }

    // Directive
    if (*m_input == '.') {
        return lex_directive();
    }

    // Identifier (either an instruction name, a register or a label reference)
    if (is_ident_start(*m_input)) {
        return lex_identifier();
    }

    m_input += 1;// skip problematic character
    m_token_end = m_input;
    return Token::ERROR;
}

void Lexer::skip_whitespace() {
    while (is_whitespace(*m_input))
        ++m_input;
}

void Lexer::skip_comment() {
    while (!is_newline(*m_input) && *m_input != '\0')
        ++m_input;
}

Token Lexer::lex_bin_immediate() {
    assert(*m_input == '0' && (m_input[1] == 'b' || m_input[1] == 'B'));

    m_input += 2;// skip '0b'

    m_last_parsed_immediate = 0;
    while (is_bin_digit(*m_input)) {
        m_last_parsed_immediate *= 2;
        m_last_parsed_immediate += *m_input - '0';
        ++m_input;
    }

    m_token_end = m_input;
    return Token::IMMEDIATE;
}

Token Lexer::lex_hex_immediate() {
    assert(*m_input == '0' && (m_input[1] == 'x' || m_input[1] == 'X'));

    m_input += 2;// skip '0x'

    m_last_parsed_immediate = 0;
    while (is_hex_digit(*m_input)) {
        m_last_parsed_immediate *= 16;

        if (is_digit(*m_input)) {
            m_last_parsed_immediate += *m_input - '0';
        } else if (*m_input >= 'a' || *m_input <= 'f') {
            m_last_parsed_immediate += *m_input - 'a' + 10;
        } else {
            m_last_parsed_immediate += *m_input - 'A' + 10;
        }

        ++m_input;
    }

    m_token_end = m_input;
    return Token::IMMEDIATE;
}

Token Lexer::lex_dec_immediate() {
    assert(is_digit(*m_input));

    m_last_parsed_immediate = 0;
    while (is_digit(*m_input)) {
        m_last_parsed_immediate *= 10;
        m_last_parsed_immediate += *m_input - '0';
        ++m_input;
    }

    m_token_end = m_input;
    return Token::IMMEDIATE;
}

Token Lexer::lex_directive() {
    assert(*m_input == '.');

    m_input += 1;// skip '.'

    while (is_ident_cont(*m_input)) {
        ++m_input;
    }

    m_token_end = m_input;
    return Token::DIRECTIVE;
}

Token Lexer::lex_identifier() {
    assert(is_ident_start(*m_input));

    m_input += 1;// skip the first character as already checked by next()

    while (is_ident_cont(*m_input)) {
        ++m_input;
    }

    m_token_end = m_input;
    return Token::IDENTIFIER;
}
