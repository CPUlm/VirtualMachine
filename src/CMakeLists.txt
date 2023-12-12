add_library(linenoise STATIC
        linenoise.c
        linenoise.h)

add_executable(cpulm_vm
    main.cpp
    vm.cpp
    vm.hpp)

target_link_libraries(cpulm_vm PUBLIC linenoise)
