// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#include <fstream>
#include <string>

#include "lexer.hpp"
#include "parser.hpp"

std::string read_file(const std::string &path) {
    std::ifstream t(path);
    t.seekg(0, std::ios::end);
    std::size_t size = t.tellg();
    std::string buffer(size, '\0');
    t.seekg(0);
    t.read(&buffer[0], (std::streamsize) size);
    return buffer;
}

void write_file(const std::string &path, const std::vector<uint32_t> &code) {
    std::ofstream t(path, std::ios::out | std::ios::binary);
    t.write(reinterpret_cast<const char*>(&code[0]), code.size()*sizeof(uint32_t ));
}

static const char *TOKENS[] = {
        "END_OF_FILE",
        "ERROR",
        "IMMEDIATE",
        "STRING",
        "COLON",
        "MINUS",
        "DIRECTIVE",
        "IDENTIFIER"};

void lex(Lexer &lexer) {
    Token token;
    do {
        token = lexer.next();
        auto spelling = lexer.get_last_token_spelling();
        printf("%s '%.*s'\n", TOKENS[(int) token], spelling.size(), spelling.data());
    } while (token != Token::END_OF_FILE);
}

int main(int argc, char *argv[]) {
    std::string input = read_file("input.s");
    Lexer lexer(input.c_str());
    Parser parser(lexer);
    auto code = parser.parse();
    write_file("output.o", code);
}
