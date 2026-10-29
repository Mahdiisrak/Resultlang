@echo off
setlocal
if not exist build mkdir build
echo Building ResultLang compiler with GCC...
gcc -std=c11 -Wall -Wextra -O2 src\main.c -o build\resultlangc.exe
if errorlevel 1 exit /b 1
echo Build complete: build\resultlangc.exe
