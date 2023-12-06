// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#include "vm.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "linenoise.h"

std::unordered_map<std::string_view, reg_index_t> REGISTERS;

constexpr reg_index_t INVALID_REG = 0xff;

static bool is_digit(char ch) {
    return (ch >= '0' && ch <= '9');
}

static bool is_ident_start(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

static bool is_ident_cont(char ch) {
    return is_ident_start(ch) || is_digit(ch);
}

struct CommandParser {
    VM &vm;
    const char *input;

    void skip_whitespace() {
        while (*input == ' ' || *input == '\t')
            ++input;
    }

    std::string_view parse_word() {
        skip_whitespace();

        if (!is_ident_start(*input))
            return {};

        const char *begin = input;

        while (is_ident_cont(*input))
            ++input;

        return {begin, static_cast<size_t>(std::distance(begin, input))};
    }

    reg_index_t parse_register() {
        const auto word = parse_word();
        const auto it = REGISTERS.find(word);
        if (it == REGISTERS.end()) {
            throw std::runtime_error("expected a register");
        } else {
            return it->second;
        }
    }

    void expect_eoc() {
        skip_whitespace();
        if (*input != '\0') {
            throw std::runtime_error("expected end of command");
        }
    }

    void parse_command() {
        try {
            const auto command = parse_word();
            if (command.empty()) {
                throw std::runtime_error("expected a command");
            } else if (command == "reg") {
                parse_reg();
            } else if (command == "execute") {
                parse_execute();
            } else if (command == "step") {
                parse_step();
				} else if (command == "exit" || command == "quit")	{
					exit(0);
            } else {
                throw std::runtime_error("unknown command '" + std::string(command) + "'");
            }
        } catch (const std::runtime_error &error) {
            std::cerr << "ERROR: " << error.what() << std::endl;
        }
    }

    void parse_reg() {
        const auto subcommand = parse_word();
        if (subcommand == "get") {
            const reg_index_t reg = parse_register();
            expect_eoc();
            std::cout << "Unsigned: " << vm.get_reg(reg) << "\n";
            std::cout << "Signed  : " << std::make_signed_t<reg_t>(vm.get_reg(reg)) << std::endl;
        } else if (subcommand == "set") {
            const reg_index_t reg = parse_register();
            expect_eoc();
        } else {
            throw std::runtime_error("expected get or set after reg");
        }
    }

    void parse_execute() {
        expect_eoc();

        if (vm.at_end()) {
            std::cout << "program execution terminated\n";
        } else {
            vm.execute();
        }
    }

    void parse_step() {
        expect_eoc();

        if (vm.at_end()) {
            std::cout << "program execution terminated\n";
        } else {
            vm.step();
        }
    }
};

class Interface {
public:
    Interface(VM &vm)
        : m_vm(vm) {}

    void repl() {
        while (true) {
#ifndef _WIN32
            char *command;
            if ((command = linenoise("vm> "))) {
                handle_command(command);
                linenoiseFree(command);
            }
#else
            std::cout << "vm> ";
            std::string command;
            std::getline(std::cin, command);
            handle_command(command);
#endif
        }
    }

    void handle_command(const std::string &command) {
        CommandParser parser = {m_vm, command.data()};
        parser.parse_command();
    }

private:
    VM &m_vm;
};


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "ERROR: missing an input file\n");
        return EXIT_FAILURE;
    }
	 for (int iArg = 1; iArg < argc; iArg++)	{
		if (strcmp(argv[iArg],"--help") == 0 || strcmp(argv[iArg],"-h") == 0 )	{
			std::cout << "SYNOPSIS :\n  ./vm filename\n  ./vm --help\n\nCOMMANDS :\n  execute\n  step\n  reg get regName\n  reg set regName value\n  exit / quit\n";
			exit(0);
		}
	}
			

#define REG(name, i) REGISTERS.insert({name, i});
#include "regs.def"

    std::ifstream input(argv[1], std::ios::binary);
    const std::vector<char> buffer(std::istreambuf_iterator<char>(input), {});
    input.close();

    if (buffer.size() % sizeof(uint32_t) != 0) {
        fprintf(stderr, "ERROR: machine code ill-formed\n");
        return EXIT_FAILURE;
    }

    VM vm(reinterpret_cast<const uint32_t *>(buffer.data()), buffer.size() / sizeof(uint32_t));
    Interface interface(vm);
    interface.repl();
    // vm.execute();

    return EXIT_SUCCESS;
}
