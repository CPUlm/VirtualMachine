add_library(linenoise STATIC
        linenoise.c
        linenoise.h)

add_executable(cpulm_vm
    main.cpp
    disassembler.c
    repl.cpp
    repl.hpp
    vm.cpp
    vm.hpp)

target_link_libraries(cpulm_vm PUBLIC linenoise)
target_link_libraries(cpulm_vm PUBLIC SparseMemory)

add_executable(cpulm_dis disassembler.c)
target_compile_definitions(cpulm_dis PRIVATE DISASSEMBLER_AS_PROGRAM)
