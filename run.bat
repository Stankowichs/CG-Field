@echo off
setlocal

call build.bat
if errorlevel 1 exit /b 1

CGProject.exe
