// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#include "linenoise.h"
#include "repl.hpp"
#include "vm.hpp"

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include "disassembler.h"

[[noreturn]] void error(const std::string& msg) {
    std::cerr << "\x1b[1;31mERROR:\x1b[0m " << msg << "\n";
    std::exit(EXIT_FAILURE);
}
struct CommandLineArgs {
    std::vector<std::string> ram_files;
    std::vector<std::string> rom_files;
    bool use_screen = true;
} cmd_line_args = {};

void show_help_message(const char* argv0) {
    std::cout << "USAGE: " << argv0 << "[options...] input.ram input.rom\n";
}

void parse_options(int argc, char* argv[]) {
    bool stop_parsing_options = false;
    for (int i = 1 /* ignore argv0 */; i < argc; i++) {
        const std::string_view option = argv[i];
        if (!stop_parsing_options) {
            if (option == "-h" || option == "--help") {
                show_help_message(argv[0]);
                std::exit(EXIT_SUCCESS);
            } else if (option == "--no-screen") {
                cmd_line_args.use_screen = false;
                continue;
            } else if (option == "--rom") {
                if (i == argc)
                    error("missing argument to '--rom'");

                const std::string_view rom_file = argv[++i];
                cmd_line_args.rom_files.emplace_back(rom_file);
                continue;
            } else if (option == "--ram") {
                if (i == argc)
                    error("missing argument to '--ram'");

                const std::string_view ram_file = argv[++i];
                cmd_line_args.ram_files.emplace_back(ram_file);
                continue;
            } else if (option == "--") {
                stop_parsing_options = true;
                continue;
            } else if (option.starts_with("-")) {
                error(std::string("unknown option '") + option.data() + "'");
            }
        }

        if (option.ends_with(".data") || option.ends_with(".do") || option.ends_with(".ram")) {
            cmd_line_args.ram_files.emplace_back(option);
        } else if (option.ends_with(".code") || option.ends_with(".po") || option.ends_with(".rom")) {
            cmd_line_args.rom_files.emplace_back(option);
        } else {
            error(std::string("cannot determine type of file '") + option.data() + "'");
        }
    }
}

static std::vector<std::uint32_t> read_file(const std::string& filename) {
    std::FILE* file = std::fopen(filename.c_str(), "rb");
    if (file == nullptr)
        error(std::string("failed to read file '") + filename + "'");

    std::vector<std::uint32_t> result;
    std::uint32_t buffer[1024];
    size_t read_words;
    while ((read_words = std::fread(buffer, sizeof(std::uint32_t), sizeof(buffer) / sizeof(std::uint32_t), file)) > 0) {
        result.insert(result.end(), buffer, buffer + read_words);
    }

    std::fclose(file);
    return result;
}

void term_show_cursor() {
    if (cmd_line_args.use_screen)
        printf("\x1b[?25h");
}

void term_save_cursor() {
    if (cmd_line_args.use_screen)
        printf("\x1b[s");
}

void term_restore_cursor() {
    if (cmd_line_args.use_screen)
        printf("\x1b[u");
}

void term_move_cursor(int x, int y) {
    if (cmd_line_args.use_screen)
        printf("\x1b[%d;%dH", y, x);
}

void term_clear_until_end() {
    if (cmd_line_args.use_screen)
        printf("\x1b[0J");
}

int main(int argc, char* argv[]) {
    linenoiseClearScreen();

    std::ostream::sync_with_stdio(true);

    parse_options(argc, argv);

    if (cmd_line_args.rom_files.empty())
        error("missing a rom file");
    if (cmd_line_args.rom_files.size() > 1)
        error("too many rom file");
    if (cmd_line_args.ram_files.size() > 1)
        error("too many ram file");

    std::vector<std::uint32_t> rom_data = read_file(cmd_line_args.rom_files[0]);
    std::vector<std::uint32_t> ram_data;
    if (!cmd_line_args.ram_files.empty())
        ram_data = read_file(cmd_line_args.ram_files[0]);

    VM vm(rom_data, ram_data, cmd_line_args.use_screen, cmd_line_args.rom_files[0].c_str());
    REPL repl(vm);
    repl.run();

    return 0;
}
