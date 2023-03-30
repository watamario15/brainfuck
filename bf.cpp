#include "bf.hpp"

#include <string.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

void Brainfuck::reset() {
  progIndex = 0;
  inIndex = 0;
  memset(memory, 0, sizeof(memory));
  memIndex = 0;
  memLen = 1;
}

void Brainfuck::reset(unsigned progLen, const wchar_t *program, unsigned inLen, const void *input) {
  if (progLen == 0 || !program) {
    Brainfuck::program = NULL;
    Brainfuck::progLen = 0;
  } else {
    Brainfuck::program = program;
    Brainfuck::progLen = progLen;
  }
  progIndex = 0;

  if (inLen == 0 || !input) {
    Brainfuck::input = NULL;
    Brainfuck::inLen = 0;
  } else {
    Brainfuck::input = (const unsigned char *)input;
    Brainfuck::inLen = inLen;
  }
  inIndex = 0;

  memset(memory, 0, sizeof(memory));
  memIndex = 0;
  memLen = 1;
}

void Brainfuck::setBehavior(enum noinput_t noInput, bool wrapInt, bool signedness, bool debug) {
  Brainfuck::noInput = noInput;
  Brainfuck::wrapInt = wrapInt;
  Brainfuck::signedness = signedness;
  Brainfuck::debug = debug;
}

enum Brainfuck::result_t Brainfuck::next(unsigned char *output, bool *didOutput) {
  enum result_t result = RESULT_RUN;
  *didOutput = false;

  if (progIndex >= progLen) {
    return RESULT_FIN;
  }

  switch (program[progIndex]) {
    case L'>':
      if (memIndex != 65535) {
        ++memIndex;
        if (memIndex == memLen) ++memLen;
      } else {
        wsprintfW(lastError, L"%u: Instruction '>' used when the memory pointer is 65535.",
                  progIndex);
        return RESULT_ERR;
      }
      break;

    case L'<':
      if (memIndex != 0) {
        --memIndex;
      } else {
        wsprintfW(lastError, L"%u: Instruction '<' used when the memory pointer is 0.", progIndex);
        return RESULT_ERR;
      }
      break;

    // Wrap around is guaranteed for an unsigned integer.
    case L'+':
      if (wrapInt) {
        ++memory[memIndex];
      } else if (signedness) {
        if (memory[memIndex] != 0x7F) {
          ++memory[memIndex];
        } else {
          wsprintfW(lastError, L"%u: Instruction '+' used when the pointed memory is 127.",
                    progIndex);
          return RESULT_ERR;
        }
      } else {
        if (memory[memIndex] != 0xFF) {
          ++memory[memIndex];
        } else {
          wsprintfW(lastError, L"%u: Instruction '+' used when the pointed memory is 255.",
                    progIndex);
          return RESULT_ERR;
        }
      }
      break;

    // Wrap around is guaranteed for an unsigned integer.
    case L'-':
      if (wrapInt) {
        --memory[memIndex];
      } else if (signedness) {
        if (memory[memIndex] != 0x80) {
          --memory[memIndex];
        } else {
          wsprintfW(lastError, L"%u: Instruction '-' used when the pointed memory is -128.",
                    progIndex);
          return RESULT_ERR;
        }
      } else {
        if (memory[memIndex] != 0x00) {
          --memory[memIndex];
        } else {
          wsprintfW(lastError, L"%u: Instruction '-' used when the pointed memory is 0.",
                    progIndex);
          return RESULT_ERR;
        }
      }
      break;

    case L'.':
      *output = memory[memIndex];
      *didOutput = true;
      break;

    case L',':
      if (inIndex >= inLen) {
        if (noInput == NOINPUT_ZERO) {
          memory[memIndex] = 0;
        } else if (noInput == NOINPUT_FF) {
          memory[memIndex] = 255;
        } else {
          wsprintfW(lastError, L"%u: Instruction ',' used when the input stream is empty.",
                    progIndex);
          return RESULT_ERR;
        }
      } else {
        memory[memIndex] = input[inIndex];
        ++inIndex;
      }
      break;

    case L'[':
      if (memory[memIndex] == 0) {
        if (progIndex == progLen - 1) {
          wsprintfW(lastError, L"%u: No matching closing bracket.", progIndex);
          return RESULT_ERR;
        }
        unsigned nextIndex = progIndex + 1, ketCnt = 0;
        while (true) {
          if (program[nextIndex] == L']') {
            if (ketCnt == 0) {
              break;  // match
            } else {
              --ketCnt;  // recursive bracket
            }
          }
          if (program[nextIndex] == L'[') ketCnt++;
          if (nextIndex == progLen - 1) {
            wsprintfW(lastError, L"%u: No matching closing bracket.", progIndex);
            return RESULT_ERR;
          }
          ++nextIndex;
        }
        progIndex = nextIndex;
      }
      break;

    case L']':
      if (memory[memIndex] != 0) {
        if (progIndex == 0) {
          wsprintfW(lastError, L"%u: No matching opening bracket.", progIndex);
          return RESULT_ERR;
        }
        unsigned nextIndex = progIndex - 1, braCnt = 0;
        while (true) {
          if (program[nextIndex] == L'[') {
            if (braCnt == 0) {
              break;  // match
            } else {
              --braCnt;  // recursive bracket
            }
          }
          if (program[nextIndex] == L']') braCnt++;
          if (nextIndex == 0) {
            wsprintfW(lastError, L"%lu: No matching opening bracket.", (unsigned long)progIndex);
            return RESULT_ERR;
          }
          --nextIndex;
        }
        progIndex = nextIndex;
      }
      break;

    // Extended spec for debugging (breakpoint).
    case L'@':
      if (debug) result = RESULT_BREAK;
      break;
  }

  ++progIndex;

  return result;
}
