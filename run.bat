@echo off
setlocal
cd /d "%~dp0"

echo ====================================
echo       ResultLang Compiler Runner
echo ====================================
echo.

if not exist program.txt (
    echo ERROR: program.txt not found.
    echo Create program.txt in this folder and write ResultLang code inside it.
    echo.
    pause
    exit /b 1
)

echo Source file: program.txt
echo.

call scripts\compile_file.bat program.txt
if errorlevel 1 (
    echo.
    echo Compilation failed. Check the compiler error above.
    echo.
    pause
    exit /b 1
)

echo.
echo Program output:
echo ------------------------------------
generated\program.exe
echo ------------------------------------
echo.
echo Done.
pause
