// Copyright (c) 2023 Hubert Gruniaux
// This file is part of asm which is released under the MIT license.
// See file LICENSE.txt for full license details.

#ifndef ASM_ASM_TOKEN_HPP
#define ASM_ASM_TOKEN_HPP

enum class Token {
    END_OF_FILE,
    ERROR,
    IMMEDIATE,
    STRING,
    COLON,
    MINUS,
    DIRECTIVE,
    IDENTIFIER
};

#endif//ASM_ASM_TOKEN_HPP
