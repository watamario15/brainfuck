@echo off
chcp 65001 >nul 2>&1

rem !!! Modify this path to fit to your environment !!!
rem You can remove this line if you add required CeGCC paths to your system.
set PATH=C:\cegcc\main\bin;C:\cegcc\main\libexec\gcc\arm-mingw32ce\9.3.0;C:\cegcc\cygwin

echo "*** Compiling C++ sources..."
g++ -Wall -Wextra -O3 -std=c++17 -c *.cpp

echo "*** Compiling res.rc..."
windres res.rc res.o

echo "*** Linking..."
g++ *.o -static -s -lmmtimer -mwindows -municode -o Brainfuck.exe

echo "OK"