add_executable(asm main.cpp
        lexer.cpp
        lexer.hpp
        token.hpp
        parser.cpp
        parser.hpp)

target_include_directories(asm PRIVATE "../common")