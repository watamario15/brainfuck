#!/bin/bash
set -e

if [ $# -ne 0 ]; then
    GCCPATH=$1/
else
    GCCPATH=
fi

echo "*** [IA-32] Compiling C++ sources..."
${GCCPATH}i686-w64-mingw32-g++ -Wall -Wextra -O3 -std=c++17 -c *.cpp

echo "*** [IA-32] Compiling res.rc..."
${GCCPATH}i686-w64-mingw32-windres resource.rc resource.o

echo "*** [IA-32] Linking..."
${GCCPATH}i686-w64-mingw32-g++ *.o -static -s -lcomctl32 -lwinmm -mwindows -municode -o Brainfuck32.exe

echo "*** [AMD64] Compiling C++ sources..."
${GCCPATH}x86_64-w64-mingw32-g++ -Wall -Wextra -O3 -std=c++17 -c *.cpp

echo "*** [AMD64] Compiling res.rc..."
${GCCPATH}x86_64-w64-mingw32-windres resource.rc resource.o

echo "*** [AMD64] Linking..."
${GCCPATH}x86_64-w64-mingw32-g++ *.o -static -s -lcomctl32 -lwinmm -mwindows -municode -o Brainfuck64.exe

echo "OK"
