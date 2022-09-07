#ifdef _MSC_VER
#define PRIu64 "I64u"
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#else
#define PRIu64 "zu"
#endif

#include "bf.hpp"

#include <cstring>
#include <stdexcept>

void bf::reset() {
  progIndex = 0;
  inIndex = 0;
  memset(memory, 0, sizeof(memory));
  memIndex = 0;
}

void bf::reset(size_t _progLen, const char *_program, size_t _inLen,
                      const unsigned char *_input) {
  if (program) delete[] program;
  if (_progLen == 0 || !_program) {
    program = NULL;
    progLen = 0;
  } else {
    program = new char[_progLen];
    memcpy(program, _program, _progLen);
    progLen = _progLen;
  }
  progIndex = 0;

  if (input) delete[] input;
  if (_inLen == 0 || !_input) {
    input = NULL;
    inLen = 0;
  } else {
    input = new unsigned char[_inLen];
    memcpy(input, _input, _inLen);
    inLen = _inLen;
  }
  inIndex = 0;

  memset(memory, 0, sizeof(memory));
  memIndex = 0;

  signedness = false;
}

void bf::reset(size_t _progLen, const char *_program, size_t _inLen,
                      const signed char *_input) {
  if (program) delete[] program;
  if (_progLen == 0 || !_program) {
    program = NULL;
    progLen = 0;
  } else {
    program = new char[_progLen];
    memcpy(program, _program, _progLen);
    progLen = _progLen;
  }
  progIndex = 0;

  if (input) delete[] input;
  if (_inLen == 0 || !_input) {
    input = NULL;
    inLen = 0;
  } else {
    input = new unsigned char[_inLen];  // reinterpret later
    memcpy(input, _input, _inLen);
    inLen = _inLen;
  }
  inIndex = 0;

  memset(memory, 0, sizeof(memory));
  memIndex = 0;

  signedness = true;
}

void bf::setBehavior(enum noinput_t _noInput, bool _wrapInt, bool _wrapPtr) {
  noInput = _noInput;
  wrapInt = _wrapInt;
  wrapPtr = _wrapPtr;
}

bool bf::next(unsigned char *_output, bool *_didOutput) {
  if (signedness) {
    throw std::invalid_argument(
        "Call this function after resetting with the unsigned version of `reset`.");
  }

  *_didOutput = false;

  if (progIndex >= progLen) {
    return true;
  }

  switch (program[progIndex]) {
    case '>':
      if (memIndex != sizeof(memory) - 1) {
        ++memIndex;
      } else if (wrapPtr) {
        memIndex = 0;
      } else {
        char errorMsg[128];
        sprintf(errorMsg, "%" PRIu64 ": Instruction '>' used when the memory pointer is 29999.",
                progIndex);
        throw std::invalid_argument(errorMsg);
      }
      break;

    case '<':
      if (memIndex != 0) {
        --memIndex;
      } else if (wrapPtr) {
        memIndex = sizeof(memory) - 1;
      } else {
        char errorMsg[128];
        sprintf(errorMsg, "%" PRIu64 ": Instruction '<' used when the memory pointer is 0.",
                progIndex);
        throw std::invalid_argument(errorMsg);
      }
      break;

    case '+':
      if (memory[memIndex] < 255) {
        ++memory[memIndex];
      } else if (wrapInt) {
        memory[memIndex] = 0;
      } else {
        char errorMsg[128];
        sprintf(errorMsg, "%" PRIu64 ": Instruction '+' used when the pointed memory is 255.",
                progIndex);
        throw std::invalid_argument(errorMsg);
      }
      break;

    case '-':
      if (memory[memIndex] > 0) {
        --memory[memIndex];
      } else if (wrapInt) {
        memory[memIndex] = 255;
      } else {
        char errorMsg[128];
        sprintf(errorMsg, "%" PRIu64 ": Instruction '-' used when the pointed memory is 0.",
                progIndex);
        throw std::invalid_argument(errorMsg);
      }
      break;

    case '.':
      *_output = memory[memIndex];
      *_didOutput = true;
      break;

    case ',':
      if (inIndex >= inLen) {
        if (noInput == NOINPUT_ZERO) {
          memory[memIndex] = 0;
        } else if (noInput == NOINPUT_FF) {
          memory[memIndex] = 255;
        } else {
          char errorMsg[128];
          sprintf(errorMsg, "%" PRIu64 ": Instruction ',' used when the input stream is empty.",
                  progIndex);
          throw std::invalid_argument(errorMsg);
        }
      } else {
        memory[memIndex] = input[inIndex];
        ++inIndex;
      }
      break;

    case '[':
      if (memory[memIndex] == 0) {
        size_t bracketIndex = progIndex++;
        int ketCnt = 0;
        while (progIndex < progLen) {
          if (program[progIndex] == ']') {
            if (ketCnt == 0) {
              break;  // match
            } else {
              --ketCnt;  // recursive bracket
            }
          }
          if (program[progIndex] == '[') ketCnt++;
          ++progIndex;
        }
        if (progIndex >= progLen) {
          char errorMsg[128];
          sprintf(errorMsg, "%" PRIu64 ": No matching closing bracket.", bracketIndex);
          throw std::invalid_argument(errorMsg);
        }
      }
      break;

    case ']':
      if (memory[memIndex] != 0) {
        size_t bracketIndex = progIndex--;
        int braCnt = 0;
        while (true) {
          if (program[progIndex] == '[') {
            if (braCnt == 0) {
              break;  // match
            } else {
              --braCnt;  // recursive bracket
            }
          }
          if (program[progIndex] == ']') braCnt++;
          if (progIndex == 0) break;
          --progIndex;
        }
        if (progIndex <= 0 && (program[progIndex] != '[' || braCnt != 0)) {
          char errorMsg[128];
          sprintf(errorMsg, "%" PRIu64 ": No matching opening bracket.", bracketIndex);
          throw std::invalid_argument(errorMsg);
        }
      }
      break;
  }

  ++progIndex;

  return false;
}

bool bf::next(signed char *_output, bool *_didOutput) {
  if (!signedness) {
    throw std::invalid_argument(
        "Call this function after resetting with the signed version of `reset`.");
  }

  *_didOutput = false;
  signed char *signedMem = (signed char *)memory, *signedIn = (signed char *)input;

  if (progIndex >= progLen) {
    return true;
  }

  switch (program[progIndex]) {
    case '>':
      if (memIndex != sizeof(memory) - 1) {
        ++memIndex;
      } else if (wrapPtr) {
        memIndex = 0;
      } else {
        char errorMsg[128];
        sprintf(errorMsg, "%" PRIu64 ": Instruction '>' used when the memory pointer is 29999.",
                progIndex);
        throw std::invalid_argument(errorMsg);
      }
      break;

    case '<':
      if (memIndex != 0) {
        --memIndex;
      } else if (wrapPtr) {
        memIndex = sizeof(memory) - 1;
      } else {
        char errorMsg[128];
        sprintf(errorMsg, "%" PRIu64 ": Instruction '<' used when the memory pointer is 0.",
                progIndex);
        throw std::invalid_argument(errorMsg);
      }
      break;

    case '+':
      if (signedMem[memIndex] < 127) {
        ++signedMem[memIndex];
      } else if (wrapInt) {
        signedMem[memIndex] = -128;
      } else {
        char errorMsg[128];
        sprintf(errorMsg, "%" PRIu64 ": Instruction '+' used when the pointed memory is 127.",
                progIndex);
        throw std::invalid_argument(errorMsg);
      }
      break;

    case '-':
      if (signedMem[memIndex] > -128) {
        --signedMem[memIndex];
      } else if (wrapInt) {
        signedMem[memIndex] = 127;
      } else {
        char errorMsg[128];
        sprintf(errorMsg, "%" PRIu64 ": Instruction '-' used when the pointed memory is -128.",
                progIndex);
        throw std::invalid_argument(errorMsg);
      }
      break;

    case '.':
      *_output = signedMem[memIndex];
      *_didOutput = true;
      break;

    case ',':
      if (inIndex >= inLen) {
        if (noInput == NOINPUT_ZERO) {
          signedMem[memIndex] = 0;
        } else if (noInput == NOINPUT_FF) {
          signedMem[memIndex] = -1;
        } else {
          char errorMsg[128];
          sprintf(errorMsg, "%" PRIu64 ": Instruction ',' used when the input stream is empty.",
                  progIndex);
          throw std::invalid_argument(errorMsg);
        }
      } else {
        signedMem[memIndex] = signedIn[inIndex];
        ++inIndex;
      }
      break;

    case '[':
      if (signedMem[memIndex] == 0) {
        size_t bracketIndex = progIndex++;
        int ketCnt = 0;
        while (progIndex < progLen) {
          if (program[progIndex] == ']') {
            if (ketCnt == 0) {
              break;  // match
            } else {
              --ketCnt;  // recursive bracket
            }
          }
          if (program[progIndex] == '[') ketCnt++;
          ++progIndex;
        }
        if (progIndex >= progLen) {
          char errorMsg[128];
          sprintf(errorMsg, "%" PRIu64 ": No matching closing bracket.", bracketIndex);
          throw std::invalid_argument(errorMsg);
        }
      }
      break;

    case ']':
      if (signedMem[memIndex] != 0) {
        size_t bracketIndex = progIndex--;
        int braCnt = 0;
        while (true) {
          if (program[progIndex] == '[') {
            if (braCnt == 0) {
              break;  // match
            } else {
              --braCnt;  // recursive bracket
            }
          }
          if (program[progIndex] == ']') braCnt++;
          if (progIndex == 0) break;
          --progIndex;
        }
        if (progIndex <= 0 && (program[progIndex] != '[' || braCnt != 0)) {
          char errorMsg[128];
          sprintf(errorMsg, "%" PRIu64 ": No matching opening bracket.", bracketIndex);
          throw std::invalid_argument(errorMsg);
        }
      }
      break;
  }

  ++progIndex;

  return false;
}
