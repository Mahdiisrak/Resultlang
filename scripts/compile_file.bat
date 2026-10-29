@echo off
setlocal
if "%~1"=="" (
  echo Usage: scripts\compile_file.bat examples\valid\10_full_result_demo.rl
  exit /b 2
)
if not exist generated mkdir generated
if not exist build\resultlangc.exe call scripts\build.bat || exit /b 1
build\resultlangc.exe "%~1" generated
if errorlevel 1 exit /b 1
gcc -std=c11 -Wall -Wextra -Wno-unused-but-set-variable generated\generated.c -o generated\program.exe
if errorlevel 1 exit /b 1
echo Generated executable: generated\program.exe
