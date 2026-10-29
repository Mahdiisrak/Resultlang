@echo off
setlocal
cd /d "%~dp0.."
python scripts\run_user_input.py
exit /b %errorlevel%
