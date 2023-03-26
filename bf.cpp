#include "bf.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <string.h>

void Brainfuck::reset() {
  progIndex = 0;
  inIndex = 0;
  memset(memory, 0, sizeof(memory));
  memIndex = 0;
  memLen = 1;
}

void Brainfuck::reset(unsigned _progLen, const wchar_t *_program, unsigned _inLen,
                      const void *_input) {
  if (_progLen == 0 || !_program) {
    program = NULL;
    progLen = 0;
  } else {
    program = _program;
    progLen = _progLen;
  }
  progIndex = 0;

  if (_inLen == 0 || !_input) {
    input = NULL;
    inLen = 0;
  } else {
    input = (const unsigned char *)_input;
    inLen = _inLen;
  }
  inIndex = 0;

  memset(memory, 0, sizeof(memory));
  memIndex = 0;
  memLen = 1;
}

void Brainfuck::setBehavior(enum noinput_t _noInput, bool _wrapInt, bool _signedness, bool _debug) {
  noInput = _noInput;
  wrapInt = _wrapInt;
  signedness = _signedness;
  debug = _debug;
}

enum Brainfuck::result_t Brainfuck::next(unsigned char *_output, bool *_didOutput) {
  enum result_t result = RESULT_RUN;
  *_didOutput = false;

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
      *_output = memory[memIndex];
      *_didOutput = true;
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
