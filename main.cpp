#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#include <windows.h>
#ifndef WS_OVERLAPPEDWINDOW
#define WS_OVERLAPPEDWINDOW \
  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
#endif

#ifdef UNDER_CE
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
#define wWinMain WinMain
#define myCreateThread(lpsa, cbStack, lpStartAddr, lpvThreadParam, fdwCreate, lpIDThread) \
  CreateThread(lpsa, cbStack, lpStartAddr, lpvThreadParam, fdwCreate, lpIDThread)
#define TTYPE DWORD
#include <commctrl.h>
#include <commdlg.h>
#else
#include <process.h>
#define myCreateThread(lpsa, cbStack, lpStartAddr, lpvThreadParam, fdwCreate, lpIDThread) \
  (HANDLE) _beginthreadex(lpsa, cbStack, lpStartAddr, lpvThreadParam, fdwCreate, lpIDThread)
#define TTYPE unsigned int
#endif

#define WND_CLASS_NAME L"brainfuck-main"
#define WM_APP_THREADEND WM_APP

#include <stdexcept>
#include <vector>

#include "bf.hpp"
#include "resource.h"
#include "ui.hpp"

// Expecting `int` to be atomic
enum ctrlthread_t : int { CTRLTHREAD_RUN, CTRLTHREAD_PAUSE, CTRLTHREAD_END };

static class Brainfuck *g_bf;
static unsigned int g_timerID = 0;
static wchar_t *g_cmdLine;
static bool g_prevCR = false;
static std::vector<unsigned char> g_outBuf;
static HANDLE g_hThread = NULL;
static volatile enum ctrlthread_t g_ctrlThread = CTRLTHREAD_RUN;

static inline bool isHex(wchar_t chr) {
  return (chr >= L'0' && chr <= L'9') || (chr >= L'A' && chr <= L'F') ||
         (chr >= L'a' && chr <= L'f');
}

// Initializes the Brainfuck module.
static bool bfInit() {
  static std::vector<unsigned char> vecIn;
  static unsigned char *input = NULL;

  ui::clearOutput();
  ui::setMemory(NULL);
  g_outBuf.clear();
  g_prevCR = false;
  int inLen;
  wchar_t *wcInput = ui::getInput();
  if (input && vecIn.empty()) delete[] input;
  input = NULL;
  vecIn.clear();

  if (ui::inCharSet == IDM_BF_INPUT_HEX) {
    wchar_t hex[2];
    int hexLen = 0;
    size_t i;
    for (i = 0; true; ++i) {
      if (isHex(wcInput[i])) {
        if (hexLen >= 2) return false;  // invalid hexadecimal input
        if (wcInput[i] > L'Z') {        // align to upper case
          hex[hexLen++] = wcInput[i] - (L'a' - L'A');
        } else {
          hex[hexLen++] = wcInput[i];
        }
      } else if (iswspace(wcInput[i]) || !wcInput[i]) {
        if (hexLen == 1) {
          if (hex[0] < L'A') {
            vecIn.push_back(hex[0] - L'0');
          } else {
            vecIn.push_back(hex[0] - L'A' + 10);
          }
          hexLen = 0;
        } else if (hexLen == 2) {
          if (hex[0] < L'A') {
            vecIn.push_back((hex[0] - L'0') << 4);
          } else {
            vecIn.push_back((hex[0] - L'A' + 10) << 4);
          }
          if (hex[1] < L'A') {
            vecIn.back() += hex[1] - L'0';
          } else {
            vecIn.back() += hex[1] - L'A' + 10;
          }
          hexLen = 0;
        }
      } else {
        return false;  // invalid hexadecimal input
      }

      if (wcInput[i] == L'\0') break;
    }
    input = vecIn.empty() ? NULL : &vecIn.front();
    inLen = vecIn.size();
  } else {
    int codePage = (ui::inCharSet == IDM_BF_INPUT_SJIS) ? 932 : CP_UTF8;
    inLen = WideCharToMultiByte(codePage, 0, wcInput, -1, (char *)NULL, 0, NULL, NULL);
    input = new unsigned char[inLen];
    WideCharToMultiByte(codePage, 0, wcInput, -1, (char *)input, inLen, NULL, NULL);
    inLen--;
  }

  g_bf->reset(wcslen(ui::getEditor()), ui::getEditor(), inLen, input);

  return true;
}

// Executes the next instruction.
static enum Brainfuck::result_t bfNext() {
  unsigned char output;
  bool didOutput;
  enum Brainfuck::result_t result;
  g_bf->setBehavior(ui::noInput, ui::wrapInt, ui::signedness, ui::breakpoint);

  try {
    result = g_bf->next(&output, &didOutput);
  } catch (std::invalid_argument const &ex) {
    if (g_timerID) {
      timeKillEvent(g_timerID);
      timeEndPeriod(ui::speed);
      g_timerID = 0;
    }
    int exLen = MultiByteToWideChar(CP_UTF8, 0, ex.what(), -1, (wchar_t *)NULL, 0);
    wchar_t *wcException = new wchar_t[exLen];
    MultiByteToWideChar(CP_UTF8, 0, ex.what(), -1, wcException, exLen);
    MessageBoxW(ui::hWnd, wcException, L"Error", MB_ICONWARNING);
    delete[] wcException;
    result = Brainfuck::RESULT_FIN;
  }

  if (result == Brainfuck::RESULT_FIN) {
    if (!g_outBuf.empty()) {
      int codePage = (ui::outCharSet == IDM_BF_OUTPUT_SJIS) ? 932 : CP_UTF8;
      if (g_outBuf.back() != 0) g_outBuf.push_back(0);  // null terminator
      int outLen = MultiByteToWideChar(codePage, 0, (char *)&g_outBuf.front(), g_outBuf.size(),
                                       (wchar_t *)NULL, 0);
      wchar_t *wcOut = new wchar_t[outLen];
      MultiByteToWideChar(codePage, 0, (char *)&g_outBuf.front(), g_outBuf.size(), wcOut, outLen);
      ui::setOutput(wcOut);
      delete[] wcOut;
    }
  } else {
    if (didOutput) {
      if (ui::outCharSet == IDM_BF_OUTPUT_ASCII) {
        wchar_t wcOut[] = {0, 0};
        MultiByteToWideChar(CP_UTF8, 0, (char *)&output, 1, wcOut, 1);
        if (g_prevCR && wcOut[0] != L'\n') ui::appendOutput(L"\n");
        if (!g_prevCR && wcOut[0] == L'\n') ui::appendOutput(L"\r");
        ui::appendOutput(wcOut);
        g_prevCR = wcOut[0] == L'\r';
      } else if (ui::outCharSet == IDM_BF_OUTPUT_HEX) {
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
        if (g_prevCR && output != '\n') g_outBuf.push_back('\n');
        if (!g_prevCR && output == '\n') g_outBuf.push_back('\r');
        g_outBuf.push_back(output);
      }
    }
  }

  return result;
}

// Executes an Brainfuck program until it completes.
TTYPE WINAPI threadRunner(LPVOID lpParameter) {
  UNREFERENCED_PARAMETER(lpParameter);

  HANDLE hEvent = 0;
  if (ui::speed != 0) {
    hEvent = CreateEventW(NULL, FALSE, TRUE, NULL);
    if (timeBeginPeriod(ui::speed) == TIMERR_NOERROR) {
      g_timerID = timeSetEvent(ui::speed, ui::speed, (LPTIMECALLBACK)hEvent, 0,
                               TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);
    }
    if (g_timerID == 0) {
      MessageBoxW(ui::hWnd, L"This speed is not supported on your device. Try slowing down.",
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
      ui::setMemory(&g_bf->getMemory());
      ui::setSelect(g_bf->getProgPtr());
    }
  }

  if (ui::speed != 0) {
    timeKillEvent(g_timerID);
    timeEndPeriod(ui::speed);
    CloseHandle(hEvent);
    g_timerID = 0;
  }
  if (g_ctrlThread == CTRLTHREAD_END) {
    g_bf->reset();
  } else {
    if (g_ctrlThread == CTRLTHREAD_RUN) {
      ui::setState(result == Brainfuck::RESULT_FIN ? ui::STATE_FINISH : ui::STATE_PAUSE);
    }
    ui::setMemory(&g_bf->getMemory());
    ui::setSelect(g_bf->getProgPtr());
  }

  PostMessageW(ui::hWnd, WM_APP_THREADEND, 0, 0);
  return 0;
}

static LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  static bool didInit = false;

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
      if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) ui::onActivate();
      break;

#ifndef UNDER_CE
    case WM_DROPFILES:
      ui::onDropFiles((HDROP)wParam);
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
      ui::onDestroy();
      PostQuitMessage(0);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDC_CMDBTN_FIRST:  // Run
          if (!didInit) {
            if (!bfInit()) {
              MessageBoxW(hWnd, L"Invalid input.", L"Error", MB_ICONWARNING);
              break;
            }
            didInit = true;
          }

          g_hThread = myCreateThread(NULL, 0, threadRunner, NULL, 0, NULL);
          if (g_hThread) {
            SetThreadPriority(g_hThread, THREAD_PRIORITY_BELOW_NORMAL);
          } else {
            MessageBoxW(hWnd, L"Failed to create a runner thread.", L"Error", MB_ICONERROR);
            break;
          }

          ui::setMemory(NULL);
          ui::setState(ui::STATE_RUN);
          break;

        case IDC_CMDBTN_FIRST + 1:  // Next
          if (!didInit) {
            if (!bfInit()) {
              MessageBoxW(hWnd, L"Invalid input.", L"Error", MB_ICONWARNING);
              break;
            }
            didInit = true;
          }

          ui::setState(bfNext() == Brainfuck::RESULT_FIN ? ui::STATE_FINISH : ui::STATE_PAUSE);
          ui::setMemory(&g_bf->getMemory());
          ui::setSelect(g_bf->getProgPtr());
          break;

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
          if (DialogBoxW(ui::hInst, L"MemViewOpt", hWnd, ui::memViewProc) == IDOK &&
              ui::state != ui::STATE_INIT) {
            ui::setMemory(&g_bf->getMemory());
          }
          break;

        case IDM_OPT_TRACK:
          ui::debug = !ui::debug;
          break;

        case IDM_OPT_HLTPROG:
          ui::setSelect(g_bf->getProgPtr());
          break;

        case IDM_OPT_HLTMEM:
          ui::setSelectMemView(g_bf->getMemPtr());
          break;

        case IDM_OPT_WORDWRAP:
          ui::switchWordwrap();
          break;

        case IDM_ABOUT:
          MessageBoxW(hWnd, APP_NAME L" version " APP_VERSION, L"About this software", 0);
          break;

        default:
          if (LOWORD(wParam) >= IDC_SCRKBD_FIRST && LOWORD(wParam) <= IDC_SCRKBD_FIRST + 8) {
            ui::onScreenKeyboard(LOWORD(wParam) - IDC_SCRKBD_FIRST);
          }
      }
      if (LOWORD(wParam) != IDC_EDITOR && LOWORD(wParam) != IDC_INPUT &&
          LOWORD(wParam) != IDC_OUTPUT && LOWORD(wParam) != IDC_MEMVIEW) {
        ui::setFocus();
      }
      break;

    default:
      return DefWindowProcW(hWnd, uMsg, wParam, lParam);
  }

  return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nShowCmd) {
  size_t i;
  UNREFERENCED_PARAMETER(hPrevInstance);

  for (i = 0; lpCmdLine[i]; ++i) {
    if (lpCmdLine[i] == L'/') lpCmdLine[i] = L'\\';
  }
  g_cmdLine = lpCmdLine;

  WNDCLASSW wcl;
  wcl.hInstance = hInstance;
  wcl.lpszClassName = WND_CLASS_NAME;
  wcl.lpfnWndProc = wndProc;
  wcl.style = 0;
  wcl.hIcon = NULL;
#ifdef UNDER_CE
  wcl.hCursor = NULL;
#else
  wcl.hCursor = LoadCursorW(NULL, IDC_ARROW);
#endif
  wcl.lpszMenuName = NULL;
  wcl.cbClsExtra = 0;
  wcl.cbWndExtra = 0;
  wcl.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
  if (!RegisterClassW(&wcl)) return FALSE;

  HWND hWnd = CreateWindowExW(0, WND_CLASS_NAME, APP_NAME, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                              CW_USEDEFAULT, CW_USEDEFAULT, 480, 320, NULL, NULL, hInstance, NULL);
  if (!hWnd) return FALSE;

  ShowWindow(hWnd, nShowCmd);
#ifdef UNDER_CE
  ShowWindow(hWnd, SW_MAXIMIZE);
#else
  DragAcceptFiles(hWnd, TRUE);
#endif
  UpdateWindow(hWnd);

  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  return (int)msg.wParam;
}
