cmake_minimum_required(VERSION 3.16)

project(cpulm_vm
        VERSION 0.1.0
        DESCRIPTION "The CPUlm virtual machine.")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(src)
