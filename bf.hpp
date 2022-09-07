#ifndef BF_HPP_
#define BF_HPP_

#include <stddef.h>

class bf {
  char *program;                // Program
  unsigned char *input;         // Input stream
  unsigned char memory[30000];  // Memory
  size_t memIndex, progIndex, inIndex, progLen, inLen;
  bool signedness, wrapInt, wrapPtr;

 public:
  enum noinput_t { NOINPUT_ERROR, NOINPUT_ZERO, NOINPUT_FF };

 private:
  enum noinput_t noInput;

 public:
  // [Universal] Initialize the internal state.
  bf()
      : program(NULL),
        input(NULL),
        signedness(true),
        wrapInt(true),
        wrapPtr(false),
        noInput(NOINPUT_ZERO) {
    reset();
  }

  // [Unsigned] Initialize the internal state and copies `_program` and `_input`.
  bf(size_t _progLen, const char *_program, size_t _inLen, const unsigned char *_input)
      : program(NULL), input(NULL), wrapInt(true), wrapPtr(false), noInput(NOINPUT_ZERO) {
    reset(_progLen, _program, _inLen, _input);
  }

  // [Signed] Initialize the internal state and copies `_program` and `_input`.
  bf(size_t _progLen, const char *_program, size_t _inLen, const signed char *_input)
      : program(NULL), input(NULL), wrapInt(true), wrapPtr(false), noInput(NOINPUT_ZERO) {
    reset(_progLen, _program, _inLen, _input);
  }

  // [Universal] Frees some resources.
  ~bf() {
    if (program) delete[] program;
    if (input) delete[] input;
  }

  // [Universal] Resets the internal state.
  void reset();

  // [Unsigned] Resets the internal state and copies `_program` and `_input`.
  void reset(size_t _progLen, const char *_program, size_t _inLen, const unsigned char *_input);

  // [Signed] Resets the internal state and copies `_program` and `_input`.
  void reset(size_t _progLen, const char *_program, size_t _inLen, const signed char *_input);

  // [Universal] Change implementation-defined behaviors.
  // Default: No input: zero, Wrap around integer: true, Wrap around pointer: false.
  void setBehavior(enum noinput_t _noInput = NOINPUT_ZERO, bool _wrapInt = true,
                   bool _wrapPtr = false);

  // [Unsigned] Executes the next code, and writes its output on `output` if any.
  // A return value indicates whether the program ended or not.
  // Throws `std::invalid_argument` when given Brainfuck program has an error.
  bool next(unsigned char *_output, bool *_didOutput);

  // [Signed] Executes the next code, and writes its output on `output` if any.
  // A return value indicates whether the program ended or not.
  // Throws `std::invalid_argument` when given Brainfuck program has an error.
  bool next(signed char *_output, bool *_didOutput);

  // [Universal] Returns the memory.
  const void *getMemory() { return (void *)memory; }
};
#endif
