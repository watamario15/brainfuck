#include <string>

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

enum ctrlthread_t { CTRLTHREAD_RUN, CTRLTHREAD_PAUSE, CTRLTHREAD_END };

static class Brainfuck *g_bf;
static unsigned int g_timerID = 0;
static wchar_t *g_cmdLine;
static bool g_prevCR = false;
static std::string g_outBuf;
static HANDLE g_hThread = NULL;
static volatile enum ctrlthread_t g_ctrlThread = CTRLTHREAD_RUN;

static inline bool isHex(wchar_t chr) {
  return (chr >= L'0' && chr <= L'9') || (chr >= L'A' && chr <= L'F') ||
         (chr >= L'a' && chr <= L'f');
}

// Initializes the Brainfuck module. Returns false on an invalid hexadecimal input.
static bool bfInit() {
  static std::string inBuf;
  static const char *input = NULL;

  ui::setOutput(L"");
  ui::setMemory(NULL);
  g_outBuf = "";
  g_prevCR = false;
  int inLen;
  wchar_t *wcInput = ui::getInput();
  // We shouldn't delete when inBuf isn't empty, as it means input is allocated by inBuf.
  if (input && inBuf.empty()) delete[] (char *)input;
  input = NULL;
  inBuf = "";

  if (ui::inCharSet == IDM_BF_INPUT_HEX) {
    // Converts the hexadecimal input.
    wchar_t hex[2];
    int hexLen = 0;
    size_t i;
    for (i = 0; true; ++i) {
      if (isHex(wcInput[i])) {
        if (hexLen >= 2) return false;  // Exceeding the 8-bit range.
        if (wcInput[i] > L'Z') {        // Aligns to the upper case.
          hex[hexLen++] = wcInput[i] - (L'a' - L'A');
        } else {
          hex[hexLen++] = wcInput[i];
        }
      } else if (iswspace(wcInput[i]) || !wcInput[i]) {
        if (hexLen == 1) {
          if (hex[0] < L'A') {
            inBuf += (char)(hex[0] - L'0');
          } else {
            inBuf += (char)(hex[0] - L'A' + 10);
          }
          hexLen = 0;
        } else if (hexLen == 2) {
          if (hex[0] < L'A') {
            inBuf += (char)((hex[0] - L'0') << 4);
          } else {
            inBuf += (char)((hex[0] - L'A' + 10) << 4);
          }
          if (hex[1] < L'A') {
            inBuf[inBuf.size() - 1] += hex[1] - L'0';
          } else {
            inBuf[inBuf.size() - 1] += hex[1] - L'A' + 10;
          }
          hexLen = 0;
        }
      } else {
        return false;  // Invalid hexadecimal input.
      }

      if (wcInput[i] == L'\0') break;
    }

    input = inBuf.empty() ? NULL : inBuf.c_str();
    inLen = (int)inBuf.size();
  } else {
    int codePage = (ui::inCharSet == IDM_BF_INPUT_SJIS) ? 932 : CP_UTF8;
    inLen = WideCharToMultiByte(codePage, 0, wcInput, -1, NULL, 0, NULL, NULL);
    char *converted = new char[inLen];
    WideCharToMultiByte(codePage, 0, wcInput, -1, converted, inLen, NULL, NULL);

    input = converted;
    inLen--;
  }

  wchar_t *wcEditor = ui::getEditor();
  g_bf->reset(wcslen(wcEditor), wcEditor, inLen, input);

  return true;
}

// Executes the next instruction.
static enum Brainfuck::result_t bfNext() {
  char output;
  bool didOutput;
  g_bf->setBehavior(ui::noInput, ui::wrapInt, ui::signedness, ui::breakpoint);

  enum Brainfuck::result_t result = g_bf->next((unsigned char *)&output, &didOutput);
  if (result == Brainfuck::RESULT_ERR) {
    if (g_timerID) {
      timeKillEvent(g_timerID);
      timeEndPeriod(ui::speed);
      g_timerID = 0;
    }
    ui::messageBox(ui::hWnd, g_bf->getLastError(), L"Error", MB_ICONWARNING);
    result = Brainfuck::RESULT_FIN;
  }

  if (result == Brainfuck::RESULT_FIN) {
    if (!g_outBuf.empty()) {
      int codePage = (ui::outCharSet == IDM_BF_OUTPUT_SJIS) ? 932 : CP_UTF8;
      int outLen = MultiByteToWideChar(codePage, 0, g_outBuf.c_str(), (int)g_outBuf.size() + 1,
                                       (wchar_t *)NULL, 0);
      wchar_t *wcOut = new wchar_t[outLen];
      MultiByteToWideChar(codePage, 0, g_outBuf.c_str(), (int)g_outBuf.size() + 1, wcOut, outLen);
      ui::setOutput(wcOut);
      delete[] wcOut;
    }
  } else {
    if (didOutput) {
      if (ui::outCharSet == IDM_BF_OUTPUT_ASCII) {
        wchar_t wcOut[] = {0, 0};
        MultiByteToWideChar(CP_UTF8, 0, (char *)&output, 1, wcOut, 1);
        // Align newlines to CRLF.
        if (g_prevCR && wcOut[0] != L'\n') ui::appendOutput(L"\n");
        if (!g_prevCR && wcOut[0] == L'\n') ui::appendOutput(L"\r");
        g_prevCR = wcOut[0] == L'\r';
        ui::appendOutput(wcOut);
      } else if (ui::outCharSet == IDM_BF_OUTPUT_HEX) {
        // Converts the output to a hexadecimal string.
        wchar_t wcOut[] = {0, 0, L' ', 0};
        unsigned char high = output >> 4, low = output & 0xF;
        if (high < 10) {
          wcOut[0] = L'0' + high;
        } else {
          wcOut[0] = L'A' + (high - 10);
        }
        if (low < 10) {
          wcOut[1] = L'0' + low;
        } else {
          wcOut[1] = L'A' + (low - 10);
        }
        ui::appendOutput(wcOut);
      } else {
        // Align newlines to CRLF.
        if (g_prevCR && output != '\n') g_outBuf += '\n';
        if (!g_prevCR && output == '\n') g_outBuf += '\r';
        g_prevCR = output == L'\r';
        g_outBuf += output;
      }
    }
  }

  return result;
}

// Executes an Brainfuck program until it completes.
tret_t WINAPI threadRunner(void *lpParameter) {
  UNREFERENCED_PARAMETER(lpParameter);

  HANDLE hEvent = 0;
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
      const unsigned char *memory = g_bf->getMemory(&size);
      ui::setMemory(memory, size);
      ui::selProg(g_bf->getProgPtr());
    }
  }

  if (g_timerID) {
    timeKillEvent(g_timerID);
    timeEndPeriod(ui::speed);
    CloseHandle(hEvent);
    g_timerID = 0;
  }
  if (g_ctrlThread == CTRLTHREAD_END) {  // Terminated
    g_bf->reset();
  } else {                                 // Paused, finished, or error
    if (g_ctrlThread == CTRLTHREAD_RUN) {  // Finished or error
      ui::setState(result == Brainfuck::RESULT_FIN ? ui::STATE_FINISH : ui::STATE_PAUSE);
    }
    unsigned size;
    const unsigned char *memory = g_bf->getMemory(&size);
    ui::setMemory(memory, size);
    ui::selProg(g_bf->getProgPtr());
  }

  PostMessageW(ui::hWnd, WM_APP_THREADEND, 0, 0);
  return 0;
}

static LRESULT CALLBACK wndProc(HWND hWnd, unsigned int uMsg, WPARAM wParam, LPARAM lParam) {
  static bool didInit = false;
  static HBRUSH BGDark = CreateSolidBrush(0x3f3936);

  switch (uMsg) {
    case WM_CREATE:
      g_bf = new Brainfuck();
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
      delete g_bf;
      DeleteObject(BGDark);
      ui::onDestroy();
      PostQuitMessage(0);
      break;

    case WM_COMMAND:
      ui::updateFocus(LOWORD(wParam));

      switch (LOWORD(wParam)) {
        case IDC_CMDBTN_FIRST:  // Run
          if (!didInit) {
            if (!bfInit()) {
              ui::messageBox(hWnd, L"Invalid input.", L"Error", MB_ICONWARNING);
              break;
            }
            didInit = true;
          }

          g_hThread = myCreateThread(NULL, 0, threadRunner, NULL, 0, NULL);
          if (g_hThread) {
            SetThreadPriority(g_hThread, THREAD_PRIORITY_BELOW_NORMAL);
          } else {
            ui::messageBox(hWnd, L"Failed to create a runner thread.", L"Error", MB_ICONERROR);
            break;
          }

          ui::setMemory(NULL);
          ui::setState(ui::STATE_RUN);
          break;

        case IDC_CMDBTN_FIRST + 1: {  // Next
          if (!didInit) {
            if (!bfInit()) {
              ui::messageBox(hWnd, L"Invalid input.", L"Error", MB_ICONWARNING);
              break;
            }
            didInit = true;
          }

          ui::setState(bfNext() == Brainfuck::RESULT_FIN ? ui::STATE_FINISH : ui::STATE_PAUSE);
          unsigned size;
          const unsigned char *memory = g_bf->getMemory(&size);
          ui::setMemory(memory, size);
          ui::selProg(g_bf->getProgPtr());
          break;
        }

        case IDC_CMDBTN_FIRST + 2:  // Pause
          if (g_hThread) g_ctrlThread = CTRLTHREAD_PAUSE;
          ui::setState(ui::STATE_PAUSE);
          break;

        case IDC_CMDBTN_FIRST + 3:  // End
          if (g_hThread) {
            g_ctrlThread = CTRLTHREAD_END;
          } else {
            g_bf->reset();
          }
          ui::setState(ui::STATE_INIT);
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
          g_bf->setBehavior(ui::noInput, ui::wrapInt, ui::signedness, ui::breakpoint);
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
            const unsigned char *memory = g_bf->getMemory(&size);
            ui::setMemory(memory, size);
          }
          break;

        case IDM_OPT_TRACK:
          ui::debug = !ui::debug;
          break;

        case IDM_OPT_HLTPROG:
          ui::selProg(g_bf->getProgPtr());
          break;

        case IDM_OPT_HLTMEM:
          ui::selMemView(g_bf->getMemPtr());
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
  size_t i;
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
