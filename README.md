# ResultLang

ResultLang is a small exam-result compiler project for CSE 303 Compiler Design at MIST. A user can write exam-related statements for a student, subject marks, total, average, grade decisions, and output. The compiler translates the ResultLang source into C, builds that C with GCC, and runs the executable.

## Features

Identifiers, student/string values, numeric marks, booleans, assignment, arithmetic `+ - * /`, parentheses, comparisons, `if/else`, `show(...)`, AST output, symbol table, TAC, generated C, GCC executable generation, and lexical/syntax/semantic diagnostics with line numbers.

## Syntax

```resultlang
student "Rahim";
math = 85;
english = 78;
science = 90;
total = math + english + science;
average = total / 3;
if (average >= 80)
  grade = "A+";
else
  grade = "B";
show(grade);
```

## Prerequisites

Windows: install GCC through MSYS2/MinGW and make sure `gcc.exe` is on PATH. Flex/Bison are documented through `src/lexer.l` and `src/parser.y`; the verified build uses the self-contained C implementation in `src/main.c` because Flex/Bison were not available in the final test environment.

## Commands

```bat
run.bat
scripts\build.bat
scripts\compile_file.bat examples\valid\10_full_result_demo.rl
generated\program.exe
scripts\run_user_input.bat
python scripts\test_all.py
```

## Easiest Way To Run

Open `program.txt`, write your ResultLang code, then double-click `run.bat`.

The runner will:

1. Build the ResultLang compiler.
2. Compile `program.txt`.
3. Generate `generated/generated.c`.
4. Build `generated/program.exe`.
5. Run the executable and show the output.

## Run Your Own ResultLang Program

Use the interactive runner:

```bat
scripts\run_user_input.bat
```

Type ResultLang code line by line, then type `END` on a new line. The script saves your code, runs the compiler, builds generated C with GCC, and executes the final program.

## Output Files

`generated/ast.txt`, `generated/symbol_table.csv`, `generated/intermediate.tac`, `generated/generated.c`, and `generated/program.exe`.

## Contents

1. [Project overview](#resultlang)
2. [Language example](#syntax)
3. [Building and running](#commands)
4. [Result representation and bit width](#result-representation-and-bit-width)
5. [Connecting a mobile device on the same IP network](#connecting-a-mobile-device-on-the-same-ip-network)
6. [Limitations](#known-limitations)

## Result Representation and Bit Width

The verified build runs on the Windows computer and generates C code. In the generated C, numeric marks, totals, and averages use `double`, which is 64 bits (IEEE-754 binary64) on the tested GCC/64-bit Windows environment. Boolean values use C `int`, which is normally 32 bits in that environment. The final grade is stored as `const char *`: the pointer is normally 64 bits on a 64-bit computer, while the text itself uses one byte per ASCII character plus a terminating byte, so its storage is variable rather than a fixed number of bits.

The mobile device does not execute the Windows `.exe` in this project. It accesses the project or report over the local network, so the ResultLang values remain represented by the desktop compiler. If the compiler is ported and compiled natively on a mobile 64-bit ARM device, the usual sizes are also 64-bit `double`, 32-bit `int`, and a 64-bit string pointer, but the sizes must be verified on that device with `sizeof(double)`, `sizeof(int)`, and `sizeof(char *)`.

## Connecting a Mobile Device on the Same IP Network

To make the project files available to a phone or tablet, I connected both devices to the same Wi-Fi network, found the computer's local IPv4 address with `ipconfig`, and started a server from the repository root:

```bat
python -m http.server 8000 --bind 0.0.0.0
```

On the mobile device, I opened `http://COMPUTER_IP:8000/` and replaced `COMPUTER_IP` with the IPv4 address shown by `ipconfig` on the computer, for example `192.168.1.25`. This allows the phone to view `README.md`, `ResultLang_Report.html`, examples, and generated artifacts through the same local network. If Windows Firewall asks, I allowed Python on the private network. The phone and computer must remain on the same LAN, and the server must remain running; this is local file/report access, not internet hosting or mobile execution of `program.exe`.

## Known Limitations

ResultLang intentionally has no functions, arrays, loops, objects, or user input. Variables are inferred on first assignment and cannot later change to an incompatible type.

## Team

Md. Mahdi Israk Mahir (202414016) and Rafsan Jony Asif (202414019).
