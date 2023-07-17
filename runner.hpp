#ifndef RUNNER_HPP_
#define RUNNER_HPP_

#include "main.hpp"

#ifdef UNDER_CE
// The return type of a thread function.
typedef DWORD tret_t;
#else
// The return type of a thread function.
typedef unsigned int tret_t;
#endif

namespace runner {
enum ctrlthread_t { CTRLTHREAD_RUN, CTRLTHREAD_PAUSE, CTRLTHREAD_END };

extern class Brainfuck bf;
extern unsigned int timerID;
extern HANDLE hThread;
extern volatile enum ctrlthread_t ctrlThread;

// Initializes the Brainfuck module. Returns false on an invalid hexadecimal input.
bool bfInit();

// Executes the next instruction.
enum Brainfuck::result_t bfNext();

// Executes an Brainfuck program until it completes.
tret_t WINAPI threadRunner(void *lpParameter);
}  // namespace runner

#endif
