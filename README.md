# Compiler
The program compiles a pascal-like language to a register-based machine bytecode.
It features two version, the first one maps each of the variables to a place in memory
(i.e. spills all variables) and uses pre-defined registers for each of the bytecode instructions.
The second, improved version, build a cfg and then performs register allocation.
Both versions perform basic optimizations like function inlining and dead code removal.
### The grammar
```
program_all     -> procedures main
procedures      -> procedures PROCEDURE proc_head IS declarations IN commands END
                   | procedures PROCEDURE proc_head IS IN commands END
                   | ε
main            -> PROGRAM IS declarations IN commands END
                   | PROGRAM IS IN commands END
commands        -> commands command
                   | command
command         -> identifier := expression ;
                   | IF condition THEN commands ELSE commands ENDIF
                   | IF condition THEN commands ENDIF
                   | WHILE condition DO commands ENDWHILE
                   | REPEAT commands UNTIL condition ;
                   | proc_call ;
                   | READ identifier ;
                   | WRITE value ;
proc_head       -> pidentifier ( args_decl )
proc_call       -> pidentifier ( args )
declarations    -> declarations , pidentifier
                   | declarations , pidentifier [ num ]
                   | pidentifier
                   | pidentifier [ num ]
args_decl       -> args_decl , pidentifier
                   | args_decl , T pidentifier
                   | pidentifier
                   | T pidentifier
args            -> args , pidentifier
                   | pidentifier
expression      -> value
                   | value + value
                   | value - value
                   | value * value
                   | value / value
                   | value % value
condition       -> value = value
                   | value != value
                   | value > value
                   | value < value
                   | value >= value
                   | value <= value
value           -> num
                   | identifier
identifier      -> pidentifier
                   | pidentifier [ num ]
                   | pidentifier [ pidentifier ]
```
### The virtual machine
The virtual machine features 8 registers, $(r_a, r_b, r_c, r_d, r_e, r_f, r_g, r_h)$, a program counter $k$
and $2^{62} + 1$ memory cells.
Values stored in the registers and the memory are unbounded, unsigned integers.
The bytecode instructions set is as follows:
| Instruction | Description                                               | Time    |
|-------------|-----------------------------------------------------------|---------|
| READ        | takes number from stdin and stores in $r_a$, k ← k + 1    | 100     |
| WRITE       | outputs r_a to stdout, k ← k + 1                          | 100     |
| LOAD x      | ra ← prx, k ← k + 1                                       | 50      |
| STORE x     | prx ← ra, k ← k + 1                                       | 50      |
| ADD x       | ra ← ra + rx, k ← k + 1                                   | 5       |
| SUB x       | ra ← max{ra − rx, 0}, k ← k + 1                           | 5       |
| GET x       | ra ← rx, k ← k + 1                                        | 1       |
| PUT x       | rx ← ra, k ← k + 1                                        | 1       |
| RST x       | rx ← 0, k ← k + 1                                         | 1       |
| INC x       | rx ← rx + 1, k ← k + 1                                    | 1       |
| DEC x       | rx ← max{rx − 1, 0}, k ← k + 1                            | 1       |
| SHL x       | rx ← 2 ∗ rx, k ← k + 1                                    | 1       |
| SHR x       | rx ← ⌊rx/2⌋, k ← k + 1                                    | 1       |
| JUMP j      | k ← j                                                     | 1       |
| JPOS j      | if ra > 0 then k ← j, otherwise k ← k + 1                 | 1       |
| JZERO j     | if ra = 0 then k ← j, otherwise k ← k + 1                 | 1       |
| STRK x      | rx ← k, k ← k + 1                                         | 1       |
| JUMPR x     | k ← rx                                                    | 1       |
| HALT        | halt the program                                          | 0       |

# Debugger
A TUI-based tool that allows step-by-step code execution and displays the state of the internally emulated machine.
#### Usage
- **n**   - step in
- **c**   - continue
- **esc** - exit the program
- **LMB** - toggle breakpoint

When input is required, the program will focus the console window.
![Debugger](./readme/debugger_screenshot.png)

# Build
The project uses two git submodules `fmt` and `doctest`. In order to succesfully build it, both of these need to be fetched.
This can be done when cloning the project with `git clone --recursive` or after cloning it using `git submodule update --init --recursive`.

Building the compiler:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```
if you want to build the debugger as well, you have to pass the `BUILD_DEBUGGER=On` variable
when configuring cmake, i.e.
```bash
mkdir build
cd build
cmake -DBUILD_DEBUGGER=On ..
cmake --build .
```
the executables will be in `./build/src/compiler` and `./build/debugger/debugger`
