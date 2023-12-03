add_library(linenoise STATIC
        linenoise.c
        linenoise.h)

add_executable(vm main.cpp
        vm.cpp
        vm.hpp)

target_link_libraries(vm PUBLIC linenoise)
target_include_directories(vm PRIVATE "../common")
