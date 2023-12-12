// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#include "vm.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

int main(int argc, char* argv[]) {
    std::cout.sync_with_stdio(true);

    if (argc < 2) {
        std::cerr << "\x1b[1;31mERROR:\x1b[0m missing an input file\n";
        return EXIT_FAILURE;
    }

    std::ifstream input(argv[1], std::ios::binary);
    const std::vector<char> buffer(std::istreambuf_iterator<char>(input), {});
    input.close();

    if (buffer.size() % sizeof(uint32_t) != 0) {
        std::cerr << "\x1b[1;31mERROR:\x1b[0m machine code ill-formed\n";
        return EXIT_FAILURE;
    }

    VM vm(reinterpret_cast<const uint32_t*>(buffer.data()), buffer.size() / sizeof(uint32_t));

    while (true) {
        std::cout << "vm> ";
        std::string line;
        std::getline(std::cin, line);

        if (line == "quit" || line == "exit") {
            return EXIT_SUCCESS;
        } else if (line == "pc") {
            const auto pc = vm.get_pc();
            std::cout << pc << std::endl;
        } else if (line == "regs") {
            std::cout << "Registers:\n";
            for (unsigned i = 0; i < 31; ++i) {
                std::cout << "  - r0 = " << vm.get_reg(i) << "\n";
            }
        } else if (line == "step") {
            vm.step();
        } else if (line == "execute") {
            vm.execute();
        } else {
            std::cerr << "\x1b[1;31mERROR:\x1b[0m invalid command\n";
        }
    }

    return EXIT_SUCCESS;
}
