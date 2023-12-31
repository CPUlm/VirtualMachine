// A very small and simple disassembler for the CPUlm ISA.

#ifndef CPULM_UTILS_H
#define CPULM_UTILS_H

#include <stdint.h>

struct SignExtend24 {
    int32_t x : 24;
};

static inline int32_t sign_extend_24(int32_t value) {
    struct SignExtend24 s;
    return (s.x = value);
}

#endif // !CPULM_UTILS_H
