#if defined UNDER_CE || (defined _MSC_VER && _MSC_VER < 1800)
#define PRIu64 "I64u"
#else
#define PRIu64 "zu"
#endif

#include "bf.hpp"

#include <cstring>
#include <stdexcept>
#include <vector>

void Brainfuck::reset() {
  progIndex = 0;
  inIndex = 0;
  memory.clear();
  memory.push_back(0);
  memIndex = 0;
}

void Brainfuck::reset(size_t _progLen, const wchar_t *_program, size_t _inLen, const void *_input) {
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

  memory.clear();
  memory.push_back(0);
  memIndex = 0;
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
      if (memIndex != memory.max_size() - 1) {
        ++memIndex;
        if (memory.size() == memIndex) memory.push_back(0);
      } else {
        char errorMsg[128];
        sprintf(errorMsg,
                "%" PRIu64
                ": Instruction '>' used when the memory pointer is std::vector::max_size.",
                progIndex);
        throw std::invalid_argument(errorMsg);
      }
      break;

    case L'<':
      if (memIndex != 0) {
        --memIndex;
      } else {
        char errorMsg[128];
        sprintf(errorMsg, "%" PRIu64 ": Instruction '<' used when the memory pointer is 0.",
                progIndex);
        throw std::invalid_argument(errorMsg);
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
          char errorMsg[128];
          sprintf(errorMsg, "%" PRIu64 ": Instruction '+' used when the pointed memory is 127.",
                  progIndex);
          throw std::invalid_argument(errorMsg);
        }
      } else {
        if (memory[memIndex] != 0xFF) {
          ++memory[memIndex];
        } else {
          char errorMsg[128];
          sprintf(errorMsg, "%" PRIu64 ": Instruction '+' used when the pointed memory is 255.",
                  progIndex);
          throw std::invalid_argument(errorMsg);
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
          char errorMsg[128];
          sprintf(errorMsg, "%" PRIu64 ": Instruction '-' used when the pointed memory is -128.",
                  progIndex);
          throw std::invalid_argument(errorMsg);
        }
      } else {
        if (memory[memIndex] != 0x00) {
          --memory[memIndex];
        } else {
          char errorMsg[128];
          sprintf(errorMsg, "%" PRIu64 ": Instruction '-' used when the pointed memory is 0.",
                  progIndex);
          throw std::invalid_argument(errorMsg);
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

    case L'[':
      if (memory[memIndex] == 0) {
        size_t bracketIndex = progIndex++, ketCnt = 0;
        while (progIndex < progLen) {
          if (program[progIndex] == L']') {
            if (ketCnt == 0) {
              break;  // match
            } else {
              --ketCnt;  // recursive bracket
            }
          }
          if (program[progIndex] == L'[') ketCnt++;
          ++progIndex;
        }
        if (progIndex >= progLen) {
          char errorMsg[128];
          sprintf(errorMsg, "%" PRIu64 ": No matching closing bracket.", bracketIndex);
          throw std::invalid_argument(errorMsg);
        }
      }
      break;

    case L']':
      if (memory[memIndex] != 0) {
        size_t bracketIndex = progIndex--, braCnt = 0;
        while (true) {
          if (program[progIndex] == L'[') {
            if (braCnt == 0) {
              break;  // match
            } else {
              --braCnt;  // recursive bracket
            }
          }
          if (program[progIndex] == L']') braCnt++;
          if (progIndex == 0) break;
          --progIndex;
        }
        if (progIndex <= 0 && (program[progIndex] != L'[' || braCnt != 0)) {
          char errorMsg[128];
          sprintf(errorMsg, "%" PRIu64 ": No matching opening bracket.", bracketIndex);
          throw std::invalid_argument(errorMsg);
        }
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
