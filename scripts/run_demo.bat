@echo off
setlocal

cd /d "%~dp0.."

echo ====================================
echo       ResultLang Final Demo
echo ====================================
echo.

call scripts\compile_file.bat examples\valid\10_full_result_demo.rl

if errorlevel 1 (
    echo.
    echo Demo failed.
    pause
    exit /b 1
)

echo.
echo Running generated executable...
echo ------------------------------------
generated\program.exe
echo ------------------------------------
echo.
echo Demo completed successfully.
