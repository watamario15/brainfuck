@echo off
chcp 65001 >nul 2>&1

if not defined PREFIX32 (
  where i686-w64-mingw32-gcc >nul 2>&1
  if not errorlevel 1 set "PREFIX32=i686-w64-mingw32-"
)
if not defined PREFIX64 (
  where x86_64-w64-mingw32-gcc >nul 2>&1
  if not errorlevel 1 set "PREFIX64=x86_64-w64-mingw32-"
)
if not defined PREFIXA32 (
  where armv7-w64-mingw32-gcc >nul 2>&1
  if not errorlevel 1 set "PREFIXA32=armv7-w64-mingw32-"
)
if not defined PREFIXA64 (
  where aarch64-w64-mingw32-gcc >nul 2>&1
  if not errorlevel 1 set "PREFIXA64=aarch64-w64-mingw32-"
)

if defined PREFIX32 (
  echo *** [IA-32] Compiling C++ sources...
  "%PREFIX32%g++" -Wall -Wextra -O3 -c .\*.cpp
  echo *** [IA-32] Compiling resource.rc...
  "%PREFIX32%windres" -c 65001 resource.rc resource.o
  echo *** [IA-32] Linking...
  "%PREFIX32%g++" .\*.o -static -s -lcomctl32 -lmmtimer -mwindows -municode -o brainfuck-win-32.exe
)

if defined PREFIX64 (
  echo *** [AMD64] Compiling C++ sources...
  "%PREFIX64%g++" -Wall -Wextra -O3 -c .\*.cpp
  echo *** [AMD64] Compiling resource.rc...
  "%PREFIX64%windres" -c 65001 resource.rc resource.o
  echo *** [AMD64] Linking...
  "%PREFIX64%g++" .\*.o -static -s -lcomctl32 -lmmtimer -mwindows -municode -o brainfuck-win-64.exe
)

if defined PREFIXA32 (
  echo *** [AArch32] Compiling C++ sources...
  "%PREFIXA32%g++" -Wall -Wextra -O3 -c .\*.cpp
  echo *** [AArch32] Compiling resource.rc...
  "%PREFIXA32%windres" -c 65001 resource.rc resource.o
  echo *** [AArch32] Linking...
  "%PREFIXA32%g++" .\*.o -static -s -lcomctl32 -lmmtimer -mwindows -municode -o brainfuck-win-arm32.exe
)

if defined PREFIXA64 (
  echo *** [AArch64] Compiling C++ sources...
  "%PREFIXA64%g++" -Wall -Wextra -O3 -c .\*.cpp
  echo *** [AArch64] Compiling resource.rc...
  "%PREFIXA64%windres" -c 65001 resource.rc resource.o
  echo *** [AArch64] Linking...
  "%PREFIXA64%g++" .\*.o -static -s -lcomctl32 -lmmtimer -mwindows -municode -o brainfuck-win-arm64.exe
)
