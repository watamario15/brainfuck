@echo off
chcp 65001 >nul 2>&1

rem !!! Modify this path to fit to your environment !!!
rem You can remove this line if you add GCC paths to your system.
set PATH=C:\MinGW\bin

echo "*** Compiling C++ sources..."
g++ -Wall -Wextra -O3 -std=c++17 -c *.cpp

echo "*** Compiling res.rc..."
windres resource.rc resource.o

echo "*** Linking..."
g++ *.o -static -s -lwinmm -mwindows -municode -o Brainfuck.exe

echo "OK"
