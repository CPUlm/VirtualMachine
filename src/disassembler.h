// A very small and simple disassembler for the CPUlm ISA.

#ifndef CPULM_DISASSEMBLER_H
#define CPULM_DISASSEMBLER_H

#include <stdint.h>

/// @brief Disassembles a single instruction.
///
/// The result is printed to stdout. In case of bad error, an assembly comment
/// is printed showing what is the problem.
///
/// @param inst The instruction to disassemble.
/// @return 0 in case of valid instruction, 1 if invalid instruction.
int cpulm_disassemble_inst(uint32_t inst);
/// @brief Disassembles all the given file.
///
/// This mainly call cpulm_disassemble_inst() on each instruction of the file.
///
/// @param filename The path to the file to disassemble.
/// @return -1 if failed to open file, 1 in case of invalid instruction and 0 if no error.
int cpulm_disassemble_file(const char* filename);

#endif // !CPULM_DISASSEMBLER_H
