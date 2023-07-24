#!/bin/sh
set -eu

if [ -z "${PREFIX32-}" ]; then
  if type i686-w64-mingw32-gcc >/dev/null 2>&1; then
    PREFIX32=i686-w64-mingw32-
  fi
fi
if [ -z "${PREFIX64-}" ]; then
  if type x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    PREFIX64=x86_64-w64-mingw32-
  fi
fi
if [ -z "${PREFIXA32-}" ]; then
  if type armv7-w64-mingw32-gcc >/dev/null 2>&1; then
    PREFIXA32=armv7-w64-mingw32-
  fi
fi
if [ -z "${PREFIXA64-}" ]; then
  if type aarch64-w64-mingw32-gcc >/dev/null 2>&1; then
    PREFIXA64=aarch64-w64-mingw32-
  fi
fi

if [ -n "${PREFIX32-}" ]; then
  echo "*** [IA-32] Compiling C++ sources..."
  "${PREFIX32}g++" -Wall -Wextra -O3 -c ./*.cpp
  echo "*** [IA-32] Compiling resource.rc..."
  "${PREFIX32}windres" -c 65001 resource.rc resource.o
  echo "*** [IA-32] Linking..."
  "${PREFIX32}g++" ./*.o -static -s -lcomctl32 -lwinmm -mwindows -municode -o brainfuck-win-32.exe
fi

if [ -n "${PREFIX64-}" ]; then
  echo "*** [AMD64] Compiling C++ sources..."
  "${PREFIX64}g++" -Wall -Wextra -O3 -c ./*.cpp
  echo "*** [AMD64] Compiling resource.rc..."
  "${PREFIX64}windres" -c 65001 resource.rc resource.o
  echo "*** [AMD64] Linking..."
  "${PREFIX64}g++" ./*.o -static -s -lcomctl32 -lwinmm -mwindows -municode -o brainfuck-win-64.exe
fi

if [ -n "${PREFIXA32-}" ]; then
  echo "*** [AArch32] Compiling C++ sources..."
  "${PREFIXA32}g++" -Wall -Wextra -O3 -c ./*.cpp
  echo "*** [AArch32] Compiling resource.rc..."
  "${PREFIXA32}windres" -c 65001 resource.rc resource.o
  echo "*** [AArch32] Linking..."
  "${PREFIXA32}g++" ./*.o -static -s -lcomctl32 -lwinmm -mwindows -municode -o brainfuck-win-arm32.exe
fi

if [ -n "${PREFIXA64-}" ]; then
  echo "*** [AArch64] Compiling C++ sources..."
  "${PREFIXA64}g++" -Wall -Wextra -O3 -c ./*.cpp
  echo "*** [AArch64] Compiling resource.rc..."
  "${PREFIXA64}windres" -c 65001 resource.rc resource.o
  echo "*** [AArch64] Linking..."
  "${PREFIXA64}g++" ./*.o -static -s -lcomctl32 -lwinmm -mwindows -municode -o brainfuck-win-arm64.exe
fi
