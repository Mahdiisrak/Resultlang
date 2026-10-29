from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
GENERATED = ROOT / "generated"
BUILD = ROOT / "build"
USER_SOURCE = GENERATED / "user_input.rl"


def run(command):
    return subprocess.run(command, cwd=ROOT, text=True, shell=True, capture_output=True)


def show_result(result):
    if result.stdout:
        print(result.stdout, end="")
    if result.stderr:
        print(result.stderr, end="", file=sys.stderr)


def main():
    print("ResultLang Interactive Compiler", flush=True)
    print("Type your ResultLang program. Type END on a new line to compile and run.", flush=True)
    print()
    print('Example:')
    print('student "Rahim";')
    print('math = 85;')
    print('english = 78;')
    print('science = 90;')
    print('total = math + english + science;')
    print('average = total / 3;')
    print('if (average >= 80)')
    print('  grade = "A+";')
    print('else')
    print('  grade = "B";')
    print('show(grade);')
    print('END')
    print()

    lines = []
    while True:
        try:
            line = input("rl> ")
        except EOFError:
            break
        if line.strip().upper() == "END":
            break
        lines.append(line)

    if not any(line.strip() for line in lines):
        print("No ResultLang code was entered.")
        return 2

    GENERATED.mkdir(exist_ok=True)
    BUILD.mkdir(exist_ok=True)
    USER_SOURCE.write_text("\n".join(lines) + "\n", encoding="utf-8")

    print()
    print(f"Saved source: {USER_SOURCE}")
    print("Building ResultLang compiler...")
    build = run(r"gcc -std=c11 -Wall -Wextra -O2 src\main.c -o build\resultlangc.exe")
    show_result(build)
    if build.returncode != 0:
        return build.returncode

    print("Compiling your ResultLang program...")
    compile_result = run(r'build\resultlangc.exe generated\user_input.rl generated')
    show_result(compile_result)
    if compile_result.returncode != 0:
        return compile_result.returncode

    print("Building generated C executable...")
    c_build = run(r"gcc -std=c11 -Wall -Wextra -Wno-unused-but-set-variable generated\generated.c -o generated\program.exe")
    show_result(c_build)
    if c_build.returncode != 0:
        return c_build.returncode

    print()
    print("Program output:")
    print("------------------------------------")
    exe = run(r"generated\program.exe")
    show_result(exe)
    print("------------------------------------")
    return exe.returncode


if __name__ == "__main__":
    sys.exit(main())
