// This module assume `signed char` is a two's complement 8-bit integer (int8_t).
// I avoid the use of int8_t as eVC4 doesn't have stdint.h.

#ifndef BF_HPP_
#define BF_HPP_

#include <stddef.h>

#include <vector>

class Brainfuck {
 public:
  // Defines a desired behavior for input instructions with no input.
  enum noinput_t { NOINPUT_ERROR, NOINPUT_ZERO, NOINPUT_FF };

  // Execution result returned from the next() function.
  enum result_t { RESULT_RUN, RESULT_BREAK, RESULT_FIN };

  // Initializes the internal state.
  Brainfuck()
      : program(NULL),
        input(NULL),
        wrapInt(true),
        signedness(true),
        debug(false),
        noInput(NOINPUT_ZERO) {
    memory.reserve(1000);
    reset();
  }

  // Initializes the internal state and sets `_program` and `_input`.
  // DO NOT CHANGE THE CONTENTS OF THEM. You must call reset() when you update them.
  Brainfuck(size_t _progLen, const wchar_t *_program, size_t _inLen, const void *_input)
      : program(NULL),
        input(NULL),
        wrapInt(true),
        signedness(true),
        debug(false),
        noInput(NOINPUT_ZERO) {
    memory.reserve(30000);
    reset(_progLen, _program, _inLen, _input);
  }

  // Resets the internal state.
  void reset();

  // Resets the internal state and copies `_program` and `_input`.
  // DO NOT CHANGE THE CONTENTS OF THEM. You must call reset() when you update them.
  void reset(size_t _progLen, const wchar_t *_program, size_t _inLen, const void *_input);

  // Change implementation-defined behaviors.
  // When wrapInt is false, `next` throws an exception on an overflow/underflow.
  // When wrapInt is true, signedness doen't have any effect.
  // When debug is true, breakpoint instruction (@) is enabled.
  // Default options are, zero for no input, wrap around integer, signed integer (no effect here),
  // and no debug.
  void setBehavior(enum noinput_t _noInput = NOINPUT_ZERO, bool _wrapInt = true,
                   bool _signedness = true, bool _debug = false);

  // Executes the next code, and writes its output on `output` if any.
  // A return value is the result of an execution, which is running, breakpoint, and finished.
  // Throws `std::invalid_argument` when encounters an invalid Brainfuck code.
  enum result_t next(unsigned char *_output, bool *_didOutput);

  // Returns the memory. The returned content becomes invalid on reset/next.
  const std::vector<unsigned char> &getMemory() { return memory; }

  // Returns the memory pointer.
  size_t getMemPtr() { return memIndex; }

  // Returns the program pointer.
  size_t getProgPtr() { return progIndex; }

 private:
  // All initializations are in constructor, as old C++ doesn't allow it on a declaration.
  const wchar_t *program;             // Program
  const unsigned char *input;         // Input stream
  std::vector<unsigned char> memory;  // Memory
  size_t memIndex, progIndex, inIndex, progLen, inLen;
  bool wrapInt, signedness, debug;
  enum noinput_t noInput;
};
#endif
