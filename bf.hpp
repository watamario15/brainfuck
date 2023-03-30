// This module assumes `unsigned char` to be an 8-bit integer (uint8_t).
// We avoid the use of uint8_t as eVC4 doesn't have stdint.h.

#ifndef BF_HPP_
#define BF_HPP_

#include <stddef.h>

class Brainfuck {
 public:
  // Used to define a desired behavior for the input instruction when no input is given.
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

  // Initializes the internal state and registers a program and an _input.
  // You must call reset() whenever you want to modify the program and the input.
  Brainfuck(unsigned progLen, const wchar_t *program, unsigned inLen, const void *input)
      : program(NULL),
        input(NULL),
        memLen(1),
        wrapInt(true),
        signedness(true),
        debug(false),
        noInput(NOINPUT_ZERO) {
    reset(progLen, program, inLen, input);
  }

  // Resets the internal state.
  void reset();

  // Resets the internal state and registers a program and an _input.
  // You must call reset() whenever you want to modify the program and the input.
  void reset(unsigned progLen, const wchar_t *program, unsigned inLen, const void *input);

  // Change implementation-defined behaviors.
  // If wrapInt is false, next() throws an exception when overflowed or underflowed.
  // If wrapInt is true, signedness doen't have any effect.
  // If debug is true, breakpoint instruction ("@") is enabled.
  // The default behavior is [zero for no input, wrap around integer, signed integer (no effects in
  // this case), no debug].
  void setBehavior(enum noinput_t noInput = NOINPUT_ZERO, bool wrapInt = true,
                   bool signedness = true, bool debug = false);

  // Executes the next code, and writes its output on `_output` if any.
  // Returns the result of an execution, which is running, breakpoint, finished, and
  // error.
  enum result_t next(unsigned char *output, bool *didOutput);

  // Returns the memory and writes its size to `_size`.
  // The returned content becomes invalid on reset() and next().
  const unsigned char *getMemory(unsigned *size) {
    *size = memLen;
    return memory;
  }

  // Returns the value of memory pointer.
  unsigned getMemPtr() { return memIndex; }

  // Returns the value of program pointer.
  unsigned getProgPtr() { return progIndex; }

  // Returns the last error.
  const wchar_t *getLastError() { return lastError; }

 private:
  const wchar_t *program;       // Program
  const unsigned char *input;   // Input stream
  unsigned char memory[65536];  // Memory
  unsigned memIndex, progIndex, inIndex, progLen, inLen, memLen;
  bool wrapInt, signedness, debug;
  enum noinput_t noInput;
  wchar_t lastError[128];
};
#endif
