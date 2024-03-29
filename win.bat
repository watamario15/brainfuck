@echo off
chcp 65001 >nul 2>&1

rem !!! Modify this path to fit to your environment !!!
rem You can remove this line if you add GCC paths to your system.
set PATH=C:\MinGW\bin

echo "*** Compiling C++ sources..."
g++ -Wall -Wextra -O3 -std=c++98 -c *.cpp

echo "*** Compiling resource.rc..."
windres resource.rc resource.o

echo "*** Linking..."
g++ *.o -static -s -lcomctl32 -lwinmm -mwindows -municode -o brainfuck-win.exe

echo "OK"
