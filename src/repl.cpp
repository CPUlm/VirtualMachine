// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#include "repl.hpp"
#include "disassembler.h"
#include "linenoise.h"

#include <charconv>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>

static bool is_whitespace(char ch) {
    switch (ch) {
    case ' ':
    case '\t':
        return true;
    default:
        return false;
    }
}

static bool is_letter(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

enum class CommandID {
    ERROR,
    QUIT,
    HELP,
    REGS,
    FLAGS,
    BREAK,
    PC,
    DIS,
    STEP,
    EXECUTE,
    CLEAR
};

class CommandParser {
public:
    CommandParser(const char* command)
        : m_it(command) { }

    CommandID parse_command() {
        std::string_view ident = parse_ident();
        if (ident == "q" || ident == "quit" || ident == "exit") {
            return CommandID::QUIT;
        } else if (ident == "h" || ident == "help") {
            return CommandID::HELP;
        } else if (ident == "r" || ident == "reg" || ident == "regs") {
            return CommandID::REGS;
        } else if (ident == "f" || ident == "flag" || ident == "flags") {
            return CommandID::FLAGS;
        } else if (ident == "b" || ident == "break") {
            return CommandID::BREAK;
        } else if (ident == "pc") {
            return CommandID::PC;
        } else if (ident == "d" || ident == "dis" || ident == "disassembler") {
            return CommandID::DIS;
        } else if (ident == "s" || ident == "step" || ident == "next") {
            return CommandID::STEP;
        } else if (ident == "e" || ident == "execute" || ident == "exec" || ident == "continue" || ident == "cont") {
            return CommandID::EXECUTE;
        } else if (ident == "clear") {
            return CommandID::CLEAR;
        } else {
            return CommandID::ERROR;
        }
    }

    bool at_end() { return *m_it == '\0'; }

    bool expect_end() {
        skip_whitespace();
        bool at_end = *m_it == '\0';
        if (!at_end)
            printf("\x1b[1;31mERROR:\x1b[0m invalid command\n");
        return at_end;
    }

    std::string_view parse_ident() {
        skip_whitespace();
        const char* begin = m_it;
        while (is_letter(*m_it))
            ++m_it;

        return std::string_view(begin, std::distance(begin, m_it));
    }

    std::optional<uint32_t> parse_uint() {
        skip_whitespace();

        const char* end = m_it + strlen(m_it);
        std::uint32_t value;
        auto result = std::from_chars(m_it, end, value);
        if (result.ec == std::errc()) {
            m_it = result.ptr;
            return value;
        } else {
            return std::nullopt;
        }
    }

    std::optional<reg_t> parse_reg_value() {
        skip_whitespace();

        const char* end = m_it + strlen(m_it);
        std::make_signed_t<reg_t> value;
        auto result = std::from_chars(m_it, end, value);
        if (result.ec == std::errc()) {
            m_it = result.ptr;
            return value;
        } else {
            return std::nullopt;
        }
    }

    void skip_whitespace() {
        while (is_whitespace(*m_it))
            ++m_it;
    }

private:
    const char* m_it;
};

static void completion_callback(const char* line, linenoiseCompletions* lc) {
    static const char* commands[] = {
        "quit",
        "exit",
        "help",
        "regs",
        "flags",
        "break",
        "pc",
        "dis",
        "disassembler",
        "step",
        "execute",
        "continue",
        "clear"
    };

    while (is_whitespace(*line))
        ++line;

    size_t line_len = strlen(line);
    for (auto command : commands) {
        if (strncasecmp(line, command, line_len) == 0) {
            linenoiseAddCompletion(lc, command);
        }
    }
}

static char* hints_callback(const char* line, int* color, int* bold) {
    CommandParser parser(line);
    CommandID command_id = parser.parse_command();
    parser.skip_whitespace();

    *color = 2;
    size_t line_len = strlen(line);

    switch (command_id) {
    case CommandID::REGS: {
        auto reg = parser.parse_uint();
        parser.skip_whitespace();
        if (!parser.at_end())
            return nullptr;

        if (reg.has_value()) {
            if (line[line_len - 1] == ' ')
                return (char*)"[<new_value>]";
            else
                return (char*)" [<new_value>]";
        } else {
            if (line[line_len - 1] == ' ')
                return (char*)"<reg> [<new_value>]";
            else
                return (char*)" <reg> [<new_value>]";
        }
    } break;
    case CommandID::BREAK:
        if (!parser.at_end())
            return nullptr;

        if (line[line_len - 1] == ' ')
            return (char*)"<addr>";
        else
            return (char*)" <addr>";
    case CommandID::DIS:
        if (!parser.at_end())
            return nullptr;

        if (line[line_len - 1] == ' ')
            return (char*)"file";
        else
            return (char*)" file";
    case CommandID::STEP:
        if (!parser.at_end())
            return nullptr;

        if (line[line_len - 1] == ' ')
            return (char*)"<n>";
        else
            return (char*)" <n>";
    default:
        return nullptr;
    }
}

REPL::REPL(VM& vm)
    : m_vm(vm) {
}

void REPL::run() {
    linenoiseHistoryLoad("/tmp/cpulm_vm_hist.txt");
    linenoiseSetCompletionCallback(completion_callback);
    linenoiseSetHintsCallback(hints_callback);

    bool should_continue = true;
    linenoiseHistorySetMaxLen(25);
    while (should_continue) {
        const char* line = linenoise("vm> ");
        if (line == nullptr) {
            should_continue = false;
            continue;
        }

        should_continue = execute(line);
        linenoiseHistoryAdd(line);

        linenoiseFree((void*)line);
    }

    linenoiseHistorySave("/tmp/cpulm_vm_hist.txt");
}

bool REPL::execute(const char* command) {
    CommandParser parser(command);
    CommandID command_id = parser.parse_command();

    switch (command_id) {
    case CommandID::QUIT:
        parser.expect_end();
        return false;
    case CommandID::HELP:
        parser.expect_end();
        return true;
    case CommandID::REGS: {
        auto reg = parser.parse_uint();
        if (!reg.has_value()) {
            if (!parser.expect_end())
                goto error;

            // Print all registers
            printf("Registers:\n");
            for (unsigned i = 0; i < 8; ++i) {
                for (unsigned j = 0; j < 4; ++j) {
                    std::string value = std::to_string(m_vm.get_reg((j * 8) + i));
                    std::string reg = "r" + std::to_string((j * 8) + i);
                    std::size_t output_length = 7 + reg.length() + value.length();
                    printf("  - %s = %s", reg.c_str(), value.c_str());
                    while (output_length < 20) {
                        putc(' ', stdout);
                        output_length++;
                    }
                }

                putc('\n', stdout);
            }
        } else {
            if (reg.value() > 31) {
                printf("\x1b[1;31mERROR:\x1b[0m register r%d does not exist\n", reg.value());
                break;
            }

            auto value = parser.parse_reg_value();
            if (!parser.expect_end())
                goto error;

            if (value.has_value()) {
                if (reg.value() <= 1) {
                    printf("\x1b[1;31mERROR:\x1b[0m register r%d is read-only\n", reg.value());
                    break;
                }

                m_vm.set_reg(reg.value(), value.value());
                printf("Register r%d set to %d\n", reg.value(), value.value());
            } else {
                printf("Register r%d = %d\n", reg.value(), m_vm.get_reg(reg.value()));
            }
        }
    } break;
    case CommandID::FLAGS:
        if (!parser.expect_end())
            goto error;

        printf("Flags:\n");
        printf("  - Z = %d             - N = %d             - C = %d             - V = %d\n",
            m_vm.get_flag(FLAG_ZERO),
            m_vm.get_flag(FLAG_NEGATIVE),
            m_vm.get_flag(FLAG_CARRY),
            m_vm.get_flag(FLAG_OVERFLOW));

        break;
    case CommandID::BREAK: {
        auto addr = parser.parse_uint();
        if (!addr.has_value()) {
            if (!parser.expect_end())
                goto error;

            m_vm.print_breakpoints();
        } else {
            if (!parser.expect_end())
                goto error;

            m_vm.add_breakpoint(addr.value());
        }
    } break;
    case CommandID::PC:
        if (!parser.expect_end())
            goto error;

        printf("PC: %#x (%u)\n", m_vm.get_pc(), m_vm.get_pc());

        break;
    case CommandID::DIS: {
        std::string_view subcommand = parser.parse_ident();
        if (!parser.expect_end())
            goto error;

        if (subcommand == "file") {
            cpulm_disassemble_file(m_vm.get_code_filename());
        } else {
            const auto pc = m_vm.get_pc();
            const auto inst = m_vm.get_code()[pc];
            cpulm_disassemble_inst(inst, pc);
        }
    } break;
    case CommandID::STEP: {
        auto steps = parser.parse_uint().value_or(1);
        if (!parser.expect_end())
            goto error;

        if (m_vm.at_end()) {
            printf("Program already terminated.\n");
        } else {
            while (steps > 0 && !m_vm.at_end()) {
                m_vm.step();
                steps--;
            }
        }
    } break;
    case CommandID::EXECUTE:
        if (!parser.expect_end())
            goto error;

        if (m_vm.at_end()) {
            printf("Program already terminated.\n");
        } else {
            m_vm.execute();
        }

        break;
    case CommandID::CLEAR:
        linenoiseClearScreen();
        break;
    case CommandID::ERROR:
        goto error;
        break;
    }

    return true;

error:
    printf("\x1b[1;31mERROR:\x1b[0m invalid command\n");
    return true;
}
