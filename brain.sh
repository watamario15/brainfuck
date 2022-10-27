#!/bin/bash
set -e

if [ $# -ne 0 ]; then
    GCCPATH=$1/
else
    GCCPATH=
fi

echo "*** Compiling C++ sources..."
${GCCPATH}arm-mingw32ce-g++ -Wall -Wextra -O3 -std=c++17 -march=armv5tej -mcpu=arm926ej-s -c *.cpp

echo "*** Compiling res.rc..."
${GCCPATH}arm-mingw32ce-windres resource.rc resource.o

echo "*** Linking..."
${GCCPATH}arm-mingw32ce-g++ *.o -static -s -lcommctrl -lcommdlg -lmmtimer -o AppMain_.exe

echo "OK"
