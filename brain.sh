#!/bin/bash
set -e

echo "*** Compiling C++ sources..."
arm-mingw32ce-g++ -Wall -Wextra -O3 -std=c++98 -march=armv5tej -mcpu=arm926ej-s -c ./*.cpp

echo "*** Compiling resource.rc..."
arm-mingw32ce-windres resource.rc resource.o

echo "*** Linking..."
arm-mingw32ce-g++ ./*.o -static -s -lcommctrl -lcommdlg -lmmtimer -o brainfuck-wce-armv5tej.exe

echo "OK"
