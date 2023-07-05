#!/bin/bash
set -e

echo "*** [IA-32] Compiling C++ sources..."
i686-w64-mingw32-g++ -Wall -Wextra -O3 -std=c++98 -c ./*.cpp

echo "*** [IA-32] Compiling res.rc..."
i686-w64-mingw32-windres resource.rc resource.o

echo "*** [IA-32] Linking..."
i686-w64-mingw32-g++ ./*.o -static -s -lcomctl32 -lwinmm -mwindows -municode -o Brainfuck32.exe

echo "*** [AMD64] Compiling C++ sources..."
x86_64-w64-mingw32-g++ -Wall -Wextra -O3 -std=c++98 -c ./*.cpp

echo "*** [AMD64] Compiling res.rc..."
x86_64-w64-mingw32-windres resource.rc resource.o

echo "*** [AMD64] Linking..."
x86_64-w64-mingw32-g++ ./*.o -static -s -lcomctl32 -lwinmm -mwindows -municode -o Brainfuck64.exe

echo "OK"
