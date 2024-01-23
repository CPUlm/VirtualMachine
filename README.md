# The CPUlm virtual machine

## Documentation

Usage: `cpulm_vm [options] input.po input.do`

The VM start an interactive environnement. The following commands are supported:

- `step`: execute the next instruction
- `execute`: execute all instructions until end of program
- `exit` or `quit`: terminates the VM execution
- `regs`: prints all registers
- `regs 20`: prints the register r20
- `regs 20 15`: sets the register r20 to 15
- `flags`: prints the flags
- `pc`: prints the program counter (PC) register
- `dis`: disassemble the next instruction
- `dis file`: disassemble all the input file
- `break`: print the current breakpoints
- `break 5`: set a breakpoint at address 5

Many commands support aliases:
- `b` or `breakpoint` for `break`
- `r` for `regs`
- `f` for `flags`
- `e` or `continue` or `exec` for `execute`
- `d` or `disassembler` for `dis`


## Build

First, do not forget to clone recursively the GIT repository:
```sh
git submodule init
git submodule update
```

The project use CMake. You can type the following commands in the shell to compile the project:
```sh
mkdir build
cd build
cmake ..
make
```
