// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#ifndef REPL_HPP
#define REPL_HPP

#include "vm.hpp"

class REPL {
public:
    REPL(VM& vm);

    void run();

private:
    bool execute(const char* command);
    void print_regs();
    void print_reg(uint32_t index);

private:
    VM& m_vm;
};

#endif
