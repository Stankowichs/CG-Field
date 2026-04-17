@echo off
setlocal

set "CXX=C:\msys64\ucrt64\bin\g++.exe"
set "TARGET=CGProject.exe"

if not exist "%CXX%" (
    echo g++ nao encontrado em "%CXX%"
    exit /b 1
)

"%CXX%" CGProject.cpp Logic.cpp Render.cpp Audio.cpp -std=c++17 -Wall -Wextra -lfreeglut -lopengl32 -lglu32 -lgdi32 -lwinmm -o "%TARGET%"
if errorlevel 1 exit /b 1

echo Build concluido: %TARGET%
