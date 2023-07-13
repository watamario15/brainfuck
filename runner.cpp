#include "runner.hpp"

#include "tokenizer.hpp"
#include "ui.hpp"

namespace runner {
class Brainfuck bf;
unsigned int timerID = 0;
HANDLE hThread = NULL;
volatile enum ctrlthread_t ctrlThread = CTRLTHREAD_RUN;

static class UTF8Tokenizer u8Tokenizer;
static class SJISTokenizer sjisTokenizer;
static bool prevCR = false;

bool bfInit() {
  int inLen;

  int inputSize = GetWindowTextLengthW(global::hInput) + 1;
  wchar_t *wcInput = (wchar_t *)malloc(sizeof(wchar_t) * inputSize);
  if (wcInput) {
    GetWindowTextW(global::hInput, wcInput, inputSize);
  } else {
    util::messageBox(global::hWnd, global::hInst, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return false;
  }

  unsigned char *input;
  if (global::inCharSet == IDM_BF_INPUT_HEX) {
    if (!util::parseHex(global::hWnd, global::hInst, wcInput, &input, &inLen)) return false;
  } else {
    int codePage = (global::inCharSet == IDM_BF_INPUT_SJIS) ? 932 : CP_UTF8;
    inLen = WideCharToMultiByte(codePage, 0, wcInput, -1, NULL, 0, NULL, NULL);
    input = (unsigned char *)malloc(inLen);
    if (!input) {
      util::messageBox(global::hWnd, global::hInst, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
      free(wcInput);
      return false;
    }
    WideCharToMultiByte(codePage, 0, wcInput, -1, (char *)input, inLen, NULL, NULL);
    inLen--;
  }

  free(wcInput);

  if (global::outCharSet == IDM_BF_OUTPUT_UTF8) {
    u8Tokenizer.flush();
  } else if (global::outCharSet == IDM_BF_OUTPUT_SJIS) {
    sjisTokenizer.flush();
  }

  int editorSize = GetWindowTextLengthW(global::hEditor) + 1;
  wchar_t *wcEditor = (wchar_t *)malloc(sizeof(wchar_t) * editorSize);
  if (wcEditor) {
    GetWindowTextW(global::hEditor, wcEditor, editorSize);
  } else {
    util::messageBox(global::hWnd, global::hInst, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    free(input);
    return false;
  }

  bf.reset((unsigned int)wcslen(wcEditor), wcEditor, inLen, input);

  SetWindowTextW(global::hOutput, NULL);
  SetWindowTextW(global::hMemView, NULL);
  prevCR = false;

  return true;
}

enum Brainfuck::result_t bfNext() {
  unsigned char output;
  bool didOutput;

  bf.setBehavior(global::noInput, global::wrapInt, global::signedness, global::breakpoint);
  enum Brainfuck::result_t result = bf.next(&output, &didOutput);

  if (result == Brainfuck::RESULT_FIN || result == Brainfuck::RESULT_ERR) {
    if (global::outCharSet == IDM_BF_OUTPUT_UTF8) {
      const wchar_t *rest = u8Tokenizer.flush();
      if (rest) ui::appendOutput(rest);
    } else if (global::outCharSet == IDM_BF_OUTPUT_SJIS) {
      const wchar_t *rest = sjisTokenizer.flush();
      if (rest) ui::appendOutput(rest);
    }
    return result;
  }

  if (didOutput) {
    if (global::outCharSet == IDM_BF_OUTPUT_HEX) {
      wchar_t wcOut[4];
      util::toHex(output, wcOut);
      ui::appendOutput(wcOut);
    } else {
      // Align newlines to CRLF.
      if (prevCR && output != '\n') ui::appendOutput(L"\n");
      if (!prevCR && output == '\n') ui::appendOutput(L"\r");
      prevCR = output == '\r';

      if (global::outCharSet == IDM_BF_OUTPUT_UTF8) {
        const wchar_t *converted = u8Tokenizer.add(output);
        if (converted) ui::appendOutput(converted);
      } else if (global::outCharSet == IDM_BF_OUTPUT_SJIS) {
        const wchar_t *converted = sjisTokenizer.add(output);
        if (converted) ui::appendOutput(converted);
      }
    }
  }

  return result;
}

tret_t WINAPI threadRunner(void *lpParameter) {
  UNREFERENCED_PARAMETER(lpParameter);

  HANDLE hEvent = NULL;
  if (global::speed != 0) {
    hEvent = CreateEventW(NULL, FALSE, TRUE, NULL);
    if (timeBeginPeriod(global::speed) == TIMERR_NOERROR) {
      timerID = timeSetEvent(global::speed, global::speed, (LPTIMECALLBACK)hEvent, 0,
                             TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);
    }
    if (!timerID) {
      util::messageBox(global::hWnd, global::hInst, L"This speed is not supported on your device. Try slowing down.",
                       L"Error", MB_ICONERROR);
      ui::setState(global::STATE_INIT);
      PostMessageW(global::hWnd, WM_APP_THREADEND, 0, 0);
      return 1;
    }
  }

  enum Brainfuck::result_t result;
  while (ctrlThread == CTRLTHREAD_RUN) {
    if (global::speed != 0) WaitForSingleObject(hEvent, INFINITE);
    if ((result = bfNext()) != Brainfuck::RESULT_RUN) break;
    if (global::debug) {
      unsigned size;
      const unsigned char *memory = bf.getMemory(&size);
      ui::setMemory(memory, size);
      ui::selProg(bf.getProgPtr());
    }
  }

  if (timerID) {
    timeKillEvent(timerID);
    timeEndPeriod(global::speed);
    timerID = 0;
  }
  if (hEvent) CloseHandle(hEvent);

  if (ctrlThread == CTRLTHREAD_END) {
    ui::setState(global::STATE_INIT);
  } else {  // Paused, finished, error, or breakpoint
    ui::setState(result == Brainfuck::RESULT_RUN || result == Brainfuck::RESULT_BREAK ? global::STATE_PAUSE
                                                                                      : global::STATE_FINISH);
    if (result == Brainfuck::RESULT_ERR) {
      util::messageBox(global::hWnd, global::hInst, bf.getLastError(), L"Brainfuck Error", MB_ICONWARNING);
    }
    unsigned size;
    const unsigned char *memory = bf.getMemory(&size);
    ui::setMemory(memory, size);
    ui::selProg(bf.getProgPtr());
  }

  PostMessageW(global::hWnd, WM_APP_THREADEND, 0, 0);
  return 0;
}
}  // namespace runner
