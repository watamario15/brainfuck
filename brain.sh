#!/bin/sh
set -eu

PREFIX=${PREFIX-"arm-mingw32ce-"}

echo "*** [ARMv5TEJ] Compiling C++ sources..."
"${PREFIX}g++" -Wall -Wextra -O3 -march=armv5tej -mcpu=arm926ej-s -c ./*.cpp
echo "*** [ARMv5TEJ] Compiling resource.rc..."
"${PREFIX}windres" -c 65001 resource.rc resource.o
echo "*** [ARMv5TEJ] Linking..."
"${PREFIX}g++" ./*.o -static -s -lcommctrl -lcommdlg -lmmtimer -o brainfuck-wce-armv5tej.exe

echo "*** [ARMv5TE] Compiling C++ sources..."
"${PREFIX}g++" -Wall -Wextra -O3 -march=armv5te -c ./*.cpp
echo "*** [ARMv5TE] Compiling resource.rc..."
"${PREFIX}windres" -c 65001 resource.rc resource.o
echo "*** [ARMv5TE] Linking..."
"${PREFIX}g++" ./*.o -static -s -lcommctrl -lcommdlg -lmmtimer -o brainfuck-wce-armv5te.exe
