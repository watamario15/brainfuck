#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#define UNICODE
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifndef WS_OVERLAPPEDWINDOW
#define WS_OVERLAPPEDWINDOW \
  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
#endif

#ifdef UNDER_CE
// Workaround for wrong macro definitions in CeGCC.
#if SW_MAXIMIZE != 12
#undef SW_MAXIMIZE
#define SW_MAXIMIZE 12
#endif
#if SW_MINIMIZE != 6
#undef SW_MINIMIZE
#define SW_MINIMIZE 6
#endif
#if WS_MINIMIZEBOX != 0x00010000L
#undef WS_MINIMIZEBOX
#define WS_MINIMIZEBOX 0x00010000L
#endif
#if WS_MAXIMIZEBOX != 0x00020000L
#undef WS_MAXIMIZEBOX
#define WS_MAXIMIZEBOX 0x00020000L
#endif

// WinMain is already unicode on Windows CE.
#define wWinMain WinMain

// Expands to _beginthreadex on Windows PC and CreateThread on Windows CE.
#define myCreateThread(lpsa, cbStack, lpStartAddr, lpvThreadParam, fdwCreate, lpIDThread) \
  CreateThread(lpsa, cbStack, lpStartAddr, lpvThreadParam, fdwCreate, lpIDThread)

// The return type of a thread function.
typedef DWORD tret_t;

#include <commctrl.h>
#include <commdlg.h>
#else
#include <process.h>

// Expands to _beginthreadex on Windows PC and CreateThread on Windows CE.
#define myCreateThread(lpsa, cbStack, lpStartAddr, lpvThreadParam, fdwCreate, lpIDThread) \
  (HANDLE) _beginthreadex(lpsa, cbStack, lpStartAddr, lpvThreadParam, fdwCreate, lpIDThread)

// The return type of a thread function.
typedef unsigned int tret_t;
#endif

#define WND_CLASS_NAME L"brainfuck-main"
#define WM_APP_THREADEND WM_APP

#include "bf.hpp"
#include "resource.h"
#include "ui.hpp"
#include "util.hpp"
#include "tokenizer.hpp"

enum ctrlthread_t { CTRLTHREAD_RUN, CTRLTHREAD_PAUSE, CTRLTHREAD_END };

static class Brainfuck g_bf;
static class UTF8Tokenizer g_u8Tokenizer;
static class SJISTokenizer g_sjisTokenizer;
static unsigned int g_timerID = 0;
static wchar_t *g_cmdLine;
static bool g_prevCR = false;
static HANDLE g_hThread = NULL;
static volatile enum ctrlthread_t g_ctrlThread = CTRLTHREAD_RUN;

// Stops the timer identified by g_timerID if it's not 0
static inline void stopTimer() {
  if (g_timerID) {
    timeKillEvent(g_timerID);
    timeEndPeriod(ui::speed);
    g_timerID = 0;
  }
}

// Initializes the Brainfuck module. Returns false on an invalid hexadecimal input.
static bool bfInit() {
  static unsigned char *input = NULL;

  ui::setOutput(L"");
  ui::setMemory(NULL);
  g_prevCR = false;
  if (input) free(input);

  int inLen;
  wchar_t *wcInput = ui::getInput();
  if (!wcInput) return false;

  if (ui::inCharSet == IDM_BF_INPUT_HEX) {
    if (!util::parseHex(ui::hWnd, wcInput, &input)) return false;
    inLen = input ? strlen((char *)input) : 0;
  } else {
    int codePage = (ui::inCharSet == IDM_BF_INPUT_SJIS) ? 932 : CP_UTF8;
    inLen = WideCharToMultiByte(codePage, 0, wcInput, -1, NULL, 0, NULL, NULL);
    input = (unsigned char *)malloc(inLen);
    WideCharToMultiByte(codePage, 0, wcInput, -1, (char *)input, inLen, NULL, NULL);
    inLen--;
  }

  if (ui::outCharSet == IDM_BF_OUTPUT_UTF8) {
    g_u8Tokenizer.flush();
  } else if (ui::outCharSet == IDM_BF_OUTPUT_SJIS) {
    g_sjisTokenizer.flush();
  }

  wchar_t *wcEditor = ui::getEditor();
  if (!wcEditor) return false;
  g_bf.reset(wcslen(wcEditor), wcEditor, inLen, input);

  return true;
}

// Executes the next instruction.
static enum Brainfuck::result_t bfNext() {
  unsigned char output;
  bool didOutput;

  g_bf.setBehavior(ui::noInput, ui::wrapInt, ui::signedness, ui::breakpoint);
  enum Brainfuck::result_t result = g_bf.next(&output, &didOutput);

  if (result == Brainfuck::RESULT_FIN || result == Brainfuck::RESULT_ERR) {
    if (ui::outCharSet == IDM_BF_OUTPUT_UTF8) {
      const wchar_t *rest = g_u8Tokenizer.flush();
      if (rest) ui::appendOutput(rest);
    } else if (ui::outCharSet == IDM_BF_OUTPUT_SJIS) {
      const wchar_t *rest = g_sjisTokenizer.flush();
      if (rest) ui::appendOutput(rest);
    }
    return result;
  }

  if (didOutput) {
    if (ui::outCharSet == IDM_BF_OUTPUT_HEX) {
      wchar_t wcOut[4];
      util::toHex(output, wcOut);
      ui::appendOutput(wcOut);
    } else {
      // Align newlines to CRLF.
      if (g_prevCR && output != '\n') ui::appendOutput(L"\n");
      if (!g_prevCR && output == '\n') ui::appendOutput(L"\r");
      g_prevCR = output == '\r';

      if (ui::outCharSet == IDM_BF_OUTPUT_ASCII) {
        wchar_t wcOut[] = {output, 0};
        ui::appendOutput(wcOut);
      } else if (ui::outCharSet == IDM_BF_OUTPUT_UTF8) {
        const wchar_t *converted = g_u8Tokenizer.add(output);
        if (converted) ui::appendOutput(converted);
      } else if (ui::outCharSet == IDM_BF_OUTPUT_SJIS) {
        const wchar_t *converted = g_sjisTokenizer.add(output);
        if (converted) ui::appendOutput(converted);
      }
    }
  }

  return result;
}

// Executes an Brainfuck program until it completes.
tret_t WINAPI threadRunner(void *lpParameter) {
  UNREFERENCED_PARAMETER(lpParameter);

  HANDLE hEvent = NULL;
  if (ui::speed != 0) {
    hEvent = CreateEventW(NULL, FALSE, TRUE, NULL);
    if (timeBeginPeriod(ui::speed) == TIMERR_NOERROR) {
      g_timerID = timeSetEvent(ui::speed, ui::speed, (LPTIMECALLBACK)hEvent, 0,
                               TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);
    }
    if (!g_timerID) {
      ui::messageBox(ui::hWnd, L"This speed is not supported on your device. Try slowing down.",
                     L"Error", MB_ICONERROR);
      ui::setState(ui::STATE_INIT);
      PostMessageW(ui::hWnd, WM_APP_THREADEND, 0, 0);
      return 1;
    }
  }

  enum Brainfuck::result_t result;
  while (g_ctrlThread == CTRLTHREAD_RUN) {
    if (ui::speed != 0) WaitForSingleObject(hEvent, INFINITE);
    if ((result = bfNext()) != Brainfuck::RESULT_RUN) break;
    if (ui::debug) {
      unsigned size;
      const unsigned char *memory = g_bf.getMemory(&size);
      ui::setMemory(memory, size);
      ui::selProg(g_bf.getProgPtr());
    }
  }

  stopTimer();
  if (hEvent) CloseHandle(hEvent);

  if (g_ctrlThread == CTRLTHREAD_END) {
    ui::setState(ui::STATE_INIT);
  } else {  // Paused, finished, error, or breakpoint
    ui::setState(result == Brainfuck::RESULT_RUN || result == Brainfuck::RESULT_BREAK
                     ? ui::STATE_PAUSE
                     : ui::STATE_FINISH);
    if (result == Brainfuck::RESULT_ERR) {
      ui::messageBox(ui::hWnd, g_bf.getLastError(), L"Brainfuck Error", MB_ICONWARNING);
    }
    unsigned size;
    const unsigned char *memory = g_bf.getMemory(&size);
    ui::setMemory(memory, size);
    ui::selProg(g_bf.getProgPtr());
  }

  PostMessageW(ui::hWnd, WM_APP_THREADEND, 0, 0);
  return 0;
}

static LRESULT CALLBACK wndProc(HWND hWnd, unsigned int uMsg, WPARAM wParam, LPARAM lParam) {
  static bool didInit = false;
  static HBRUSH BGDark = CreateSolidBrush(0x3f3936);

  switch (uMsg) {
    case WM_CREATE:
      ui::onCreate(hWnd, ((LPCREATESTRUCT)(lParam))->hInstance);
      if (g_cmdLine[0]) {
        ui::openFile(false, g_cmdLine);
      } else {
        ui::openFile(true);
      }
      break;

    case WM_SIZE:
      ui::onSize();
      break;

    case WM_INITMENUPOPUP:
      ui::onInitMenuPopup();
      break;

    case WM_ACTIVATE:
      if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) ui::updateFocus();
      break;

    case WM_CTLCOLOREDIT:
      if (ui::dark) {
        SetTextColor((HDC)wParam, 0x00ff00);
        SetBkColor((HDC)wParam, 0x3f3936);
        return (LRESULT)BGDark;
      } else {
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
      }

    case WM_CTLCOLORSTATIC:
      if (ui::dark) {
        SetTextColor((HDC)wParam, 0x00ff00);
        SetBkColor((HDC)wParam, 0x000000);
        return (LRESULT)GetStockObject(BLACK_BRUSH);
      } else {
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
      }

#ifndef UNDER_CE
    case WM_DROPFILES:
      ui::onDropFiles((HDROP)wParam);
      break;

    case WM_GETMINMAXINFO:
      ui::onGetMinMaxInfo((MINMAXINFO *)lParam);
      break;

    case 0x2e0:  // WM_DPICHANGED
      ui::onDPIChanged(HIWORD(wParam), (const RECT *)lParam);
      break;
#endif

    case WM_APP_THREADEND:
      WaitForSingleObject(g_hThread, INFINITE);
      CloseHandle(g_hThread);
      g_hThread = NULL;
      g_ctrlThread = CTRLTHREAD_RUN;
      break;

    case WM_CLOSE:
      if (ui::promptSave()) DestroyWindow(hWnd);
      break;

    case WM_DESTROY:
      DeleteObject(BGDark);
      ui::onDestroy();
      PostQuitMessage(0);
      break;

    case WM_COMMAND:
      ui::updateFocus(LOWORD(wParam));

      switch (LOWORD(wParam)) {
        case IDC_CMDBTN_FIRST:  // Run
          if (!didInit) {
            if (!bfInit()) break;
            didInit = true;
          }

          g_hThread = myCreateThread(NULL, 0, threadRunner, NULL, 0, NULL);
          if (g_hThread) {
            SetThreadPriority(g_hThread, THREAD_PRIORITY_BELOW_NORMAL);
          } else {
            ui::messageBox(hWnd, L"Failed to create a runner thread.", L"Internal Error",
                           MB_ICONERROR);
            break;
          }

          ui::setMemory(NULL);
          ui::setState(ui::STATE_RUN);
          break;

        case IDC_CMDBTN_FIRST + 1: {  // Next
          if (!didInit) {
            if (!bfInit()) break;
            didInit = true;
          }

          Brainfuck::result_t result = bfNext();
          ui::setState(result == Brainfuck::RESULT_FIN || result == Brainfuck::RESULT_ERR
                           ? ui::STATE_FINISH
                           : ui::STATE_PAUSE);
          if (result == Brainfuck::RESULT_ERR) {
            ui::messageBox(ui::hWnd, g_bf.getLastError(), L"Brainfuck Error", MB_ICONWARNING);
          }

          unsigned size;
          const unsigned char *memory = g_bf.getMemory(&size);
          ui::setMemory(memory, size);
          ui::selProg(g_bf.getProgPtr());
          break;
        }

        case IDC_CMDBTN_FIRST + 2:  // Pause
          if (g_hThread) g_ctrlThread = CTRLTHREAD_PAUSE;
          break;

        case IDC_CMDBTN_FIRST + 3:  // End
          if (g_hThread) {
            g_ctrlThread = CTRLTHREAD_END;
          } else {
            ui::setState(ui::STATE_INIT);
          }
          didInit = false;
          break;

        case IDM_FILE_NEW:
          ui::openFile(true);
          break;

        case IDM_FILE_OPEN:
          ui::openFile(false);
          break;

        case IDM_FILE_SAVE:
          ui::saveFile(true);
          break;

        case IDM_FILE_SAVE_AS:
          ui::saveFile(false);
          break;

        case IDM_FILE_EXIT:
          SendMessageW(hWnd, WM_CLOSE, 0, 0);
          break;

        case IDM_EDIT_UNDO:
          ui::undo();
          break;

        case IDM_EDIT_CUT:
          ui::cut();
          break;

        case IDM_EDIT_COPY:
          ui::copy();
          break;

        case IDM_EDIT_PASTE:
          ui::paste();
          break;

        case IDM_EDIT_SELALL:
          ui::selAll();
          break;

        case IDM_BF_MEMTYPE_SIGNED:
          ui::signedness = true;
          break;

        case IDM_BF_MEMTYPE_UNSIGNED:
          ui::signedness = false;
          break;

        case IDM_BF_OUTPUT_ASCII:
          ui::outCharSet = IDM_BF_OUTPUT_ASCII;
          break;

        case IDM_BF_OUTPUT_UTF8:
          ui::outCharSet = IDM_BF_OUTPUT_UTF8;
          break;

        case IDM_BF_OUTPUT_SJIS:
          ui::outCharSet = IDM_BF_OUTPUT_SJIS;
          break;

        case IDM_BF_OUTPUT_HEX:
          ui::outCharSet = IDM_BF_OUTPUT_HEX;
          break;

        case IDM_BF_INPUT_UTF8:
          ui::inCharSet = IDM_BF_INPUT_UTF8;
          break;

        case IDM_BF_INPUT_SJIS:
          ui::inCharSet = IDM_BF_INPUT_SJIS;
          break;

        case IDM_BF_INPUT_HEX:
          ui::inCharSet = IDM_BF_INPUT_HEX;
          break;

        case IDM_BF_NOINPUT_ERROR:
          ui::noInput = Brainfuck::NOINPUT_ERROR;
          break;

        case IDM_BF_NOINPUT_ZERO:
          ui::noInput = Brainfuck::NOINPUT_ZERO;
          break;

        case IDM_BF_NOINPUT_FF:
          ui::noInput = Brainfuck::NOINPUT_FF;
          break;

        case IDM_BF_INTOVF_ERROR:
          ui::wrapInt = false;
          break;

        case IDM_BF_INTOVF_WRAPAROUND:
          ui::wrapInt = true;
          break;

        case IDM_BF_BREAKPOINT:
          ui::breakpoint = !ui::breakpoint;
          g_bf.setBehavior(ui::noInput, ui::wrapInt, ui::signedness, ui::breakpoint);
          break;

        case IDM_OPT_SPEED_FASTEST:
          ui::speed = 0;
          break;

        case IDM_OPT_SPEED_1MS:
          ui::speed = 1;
          break;

        case IDM_OPT_SPEED_10MS:
          ui::speed = 10;
          break;

        case IDM_OPT_SPEED_100MS:
          ui::speed = 100;
          break;

        case IDM_OPT_MEMVIEW:
          if (DialogBoxW(ui::hInst, L"memviewopt", hWnd, ui::memViewProc) == IDOK &&
              ui::state != ui::STATE_INIT) {
            unsigned size;
            const unsigned char *memory = g_bf.getMemory(&size);
            ui::setMemory(memory, size);
          }
          break;

        case IDM_OPT_TRACK:
          ui::debug = !ui::debug;
          break;

        case IDM_OPT_HLTPROG:
          ui::selProg(g_bf.getProgPtr());
          break;

        case IDM_OPT_HLTMEM:
          ui::selMemView(g_bf.getMemPtr());
          break;

        case IDM_OPT_DARK:
          ui::switchTheme();
          break;

        case IDM_OPT_LAYOUT:
          ui::switchLayout();
          break;

        case IDM_OPT_FONT:
          ui::chooseFont();
          break;

        case IDM_OPT_WORDWRAP:
          ui::switchWordwrap();
          break;

        case IDM_ABOUT:
          ui::messageBox(hWnd,
            APP_NAME L" version " APP_VERSION L"\r\n"
            APP_DESCRIPTION L"\r\n\r\n"
            APP_COPYRIGHT L"\r\n"
            L"Licensed under the MIT License.",
            L"About this software",
            0
          );
          break;

        default:
          if (LOWORD(wParam) >= IDC_SCRKBD_FIRST &&
              LOWORD(wParam) < IDC_SCRKBD_FIRST + SCRKBD_LEN) {
            ui::onScreenKeyboard(LOWORD(wParam) - IDC_SCRKBD_FIRST);
          }
      }
      break;

    default:
      return DefWindowProcW(hWnd, uMsg, wParam, lParam);
  }

  return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t *lpCmdLine,
                    int nShowCmd) {
  int i;
  UNREFERENCED_PARAMETER(hPrevInstance);

  // Replaces slashes with backslashes.
  for (i = 0; lpCmdLine[i]; ++i) {
    if (lpCmdLine[i] == L'/') lpCmdLine[i] = L'\\';
  }
  g_cmdLine = lpCmdLine;

  WNDCLASSW wcl;
  wcl.hInstance = hInstance;
  wcl.lpszClassName = WND_CLASS_NAME;
  wcl.lpfnWndProc = wndProc;
  wcl.style = 0;
#ifdef UNDER_CE
  wcl.hIcon = NULL;
  wcl.hCursor = NULL;
#else
  wcl.hIcon = LoadIconW(hInstance, L"icon");
  wcl.hCursor = LoadCursorW(NULL, IDC_ARROW);
#endif
  wcl.lpszMenuName = NULL;
  wcl.cbClsExtra = 0;
  wcl.cbWndExtra = 0;
  wcl.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);  // This is the button face color weirdly.
  if (!RegisterClassW(&wcl)) return FALSE;

  HWND hWnd = CreateWindowExW(0, WND_CLASS_NAME, APP_NAME, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                              CW_USEDEFAULT, CW_USEDEFAULT, 480, 320, NULL, NULL, hInstance, NULL);
  if (!hWnd) return FALSE;

  ShowWindow(hWnd, nShowCmd);
#ifdef UNDER_CE
  ShowWindow(hWnd, SW_MAXIMIZE);  // Maximizes as most Windows CE devices have a small display.
#else
  DragAcceptFiles(hWnd, TRUE);
#endif
  UpdateWindow(hWnd);

  HACCEL hAccel = LoadAcceleratorsW(hInstance, L"accel");
  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0)) {
    if (!TranslateAcceleratorW(hWnd, hAccel, &msg)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  return (int)msg.wParam;
}
