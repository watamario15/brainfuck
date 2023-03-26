// This module assumes `unsigned char` to be an 8-bit integer (uint8_t).
// We avoid the use of uint8_t as eVC4 doesn't have stdint.h.

#ifndef BF_HPP_
#define BF_HPP_

#include <stddef.h>

class Brainfuck {
 public:
  // Defines a desired behavior for input instructions with no input.
  enum noinput_t { NOINPUT_ERROR, NOINPUT_ZERO, NOINPUT_FF };

  // Execution result returned from the next() function.
  enum result_t { RESULT_RUN, RESULT_BREAK, RESULT_FIN, RESULT_ERR };

  // Initializes the internal state.
  Brainfuck()
      : program(NULL),
        input(NULL),
        memLen(1),
        wrapInt(true),
        signedness(true),
        debug(false),
        noInput(NOINPUT_ZERO) {
    reset();
  }

  // Initializes the internal state and sets `_program` and `_input`.
  // DO NOT CHANGE THE CONTENTS OF THEM. You must call reset() when you update them.
  Brainfuck(unsigned _progLen, const wchar_t *_program, unsigned _inLen, const void *_input)
      : program(NULL),
        input(NULL),
        memLen(1),
        wrapInt(true),
        signedness(true),
        debug(false),
        noInput(NOINPUT_ZERO) {
    reset(_progLen, _program, _inLen, _input);
  }

  // Resets the internal state.
  void reset();

  // Resets the internal state and copies `_program` and `_input`.
  // DO NOT CHANGE THE CONTENTS OF THEM. You must call reset() when you update them.
  void reset(unsigned _progLen, const wchar_t *_program, unsigned _inLen, const void *_input);

  // Change implementation-defined behaviors.
  // When wrapInt is false, `next` throws an exception on an overflow/underflow.
  // When wrapInt is true, signedness doen't have any effect.
  // When debug is true, breakpoint instruction ("@") is enabled.
  // Default options are, zero for no input, wrap around integer, signed integer (no effects in
  // this case), and no debug.
  void setBehavior(enum noinput_t _noInput = NOINPUT_ZERO, bool _wrapInt = true,
                   bool _signedness = true, bool _debug = false);

  // Executes the next code, and writes its output on `output` if any.
  // A return value is the result of an execution, which is running, breakpoint, finished, and
  // error.
  enum result_t next(unsigned char *_output, bool *_didOutput);

  // Returns the memory. The returned content becomes invalid on reset/next.
  const unsigned char *getMemory(unsigned *_size) {
    *_size = memLen;
    return memory;
  }

  // Returns the memory pointer.
  unsigned getMemPtr() { return memIndex; }

  // Returns the program pointer.
  unsigned getProgPtr() { return progIndex; }

  // Returns the last error.
  const wchar_t *getLastError() { return lastError; }

 private:
  // All initializations are in constructor, as old C++ doesn't allow them on the declaration.
  const wchar_t *program;       // Program
  const unsigned char *input;   // Input stream
  unsigned char memory[65536];  // Memory
  unsigned memIndex, progIndex, inIndex, progLen, inLen, memLen;
  bool wrapInt, signedness, debug;
  enum noinput_t noInput;
  wchar_t lastError[128];
};
#endif
