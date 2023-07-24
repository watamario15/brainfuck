@echo off
chcp 65001 >nul 2>&1

rem !!! Modify this path to fit to your environment !!!
rem You can remove this line if you add required CeGCC paths to your system.
set PATH=C:\cegcc\main\bin;C:\cegcc\main\libexec\gcc\arm-mingw32ce\9.3.0;C:\cegcc\cygwin

echo "*** Compiling C++ sources..."
arm-mingw32ce-g++ -Wall -Wextra -O3 -std=c++98 -march=armv5tej -mcpu=arm926ej-s -c *.cpp

echo "*** Compiling resource.rc..."
arm-mingw32ce-windres resource.rc resource.o

echo "*** Linking..."
arm-mingw32ce-g++ *.o -static -s -lcommctrl -lcommdlg -lmmtimer -o brainfuck-wce-armv5tej.exe

echo "OK"
