#if (defined _MSC_VER) && !(defined _CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif
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
#include <commctrl.h>
#include <commdlg.h>
#endif

#include <stdexcept>
#include <vector>

#include "bf.hpp"
#include "def.h"
#include "ui.hpp"

enum ctrlthread_t { CTRLTHREAD_RUN, CTRLTHREAD_PAUSE, CTRLTHREAD_END };

static class bf *g_bf;
static unsigned int g_timerID = 0;
static wchar_t *g_cmdLine;
static std::vector<unsigned char> g_outBuf;
static HANDLE g_hThread = NULL;
static volatile enum ctrlthread_t g_ctrlThread = CTRLTHREAD_RUN;

static inline bool isHex(wchar_t chr) {
  return (chr >= L'0' && chr <= L'9') || (chr >= L'A' && chr <= L'F') ||
         (chr >= L'a' && chr <= L'f');
}

static inline bool isSpace(wchar_t chr) {
  return chr == L' ' || chr == L'\r' || chr == L'\n' || chr == L'\t' || chr == L'\0';
}

// Initializes the Brainfuck module.
static bool bfInit() {
  ui::clearOutput();
  g_outBuf.clear();

  unsigned char *input;
  wchar_t *wcEditor = ui::getEditor(), *wcInput = ui::getInput();
  int editLen = WideCharToMultiByte(CP_UTF8, 0, wcEditor, -1, (char *)NULL, 0, NULL, NULL), inLen;
  char *szEditor = new char[editLen];
  WideCharToMultiByte(CP_UTF8, 0, wcEditor, -1, szEditor, editLen, NULL, NULL);

  std::vector<unsigned char> vecIn;
  if (ui::inCharSet == IDM_OPT_INPUT_HEX) {
    wchar_t hex[2];
    int hexLen = 0;
    size_t i;
    for (i = 0; true; ++i) {
      if (isHex(wcInput[i])) {
        if (hexLen >= 2) return false;
        if (wcInput[i] > L'Z') {  // align to upper case
          hex[hexLen++] = wcInput[i] - (L'a' - L'A');
        } else {
          hex[hexLen++] = wcInput[i];
        }
      } else if (isSpace(wcInput[i])) {
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
        return false;
      }

      if (wcInput[i] == L'\0') break;
    }
    input = vecIn.empty() ? NULL : &vecIn.front();
    inLen = vecIn.size();
  } else {
    int codePage = (ui::inCharSet == IDM_OPT_INPUT_SJIS) ? 932 : CP_UTF8;
    inLen = WideCharToMultiByte(codePage, 0, wcInput, -1, (char *)NULL, 0, NULL, NULL);
    unsigned char *szInput = new unsigned char[inLen];
    WideCharToMultiByte(codePage, 0, wcInput, -1, (char *)szInput, inLen, NULL, NULL);
    input = szInput;
    inLen--;
  }

  if (ui::signedness) {
    g_bf->reset(editLen - 1, szEditor, inLen, (signed char *)input);
  } else {
    g_bf->reset(editLen - 1, szEditor, inLen, input);
  }

  delete[] szEditor;
  if (ui::inCharSet != IDM_OPT_INPUT_HEX) delete[] input;

  return true;
}

// Executes the next instruction.
static bool bfNext(HWND hWnd, enum ui::state_t state) {
  unsigned char output;
  bool didOutput, fin;
  g_bf->setBehavior(ui::noInput, ui::wrapInt, ui::wrapPtr);

  try {
    fin = ui::signedness ? g_bf->next((signed char *)&output, &didOutput)
                         : g_bf->next(&output, &didOutput);
  } catch (std::invalid_argument const &ex) {
    if (g_timerID) {
      timeKillEvent(g_timerID);
      timeEndPeriod(ui::speed);
      g_timerID = 0;
    }
    int exLen = MultiByteToWideChar(CP_UTF8, 0, ex.what(), -1, (wchar_t *)NULL, 0);
    wchar_t *wcException = new wchar_t[exLen];
    MultiByteToWideChar(CP_UTF8, 0, ex.what(), -1, wcException, exLen);
    MessageBoxW(hWnd, wcException, L"Error", MB_ICONWARNING);
    delete[] wcException;
    fin = true;
  }

  if (fin) {
    if (!g_outBuf.empty()) {
      int codePage = (ui::outCharSet == IDM_OPT_OUTPUT_SJIS) ? 932 : CP_UTF8;
      if (g_outBuf.back() != 0) g_outBuf.push_back(0);  // null terminator
      int outLen = MultiByteToWideChar(codePage, 0, (char *)&g_outBuf.front(), g_outBuf.size(),
                                       (wchar_t *)NULL, 0);
      wchar_t *wcOut = new wchar_t[outLen];
      MultiByteToWideChar(codePage, 0, (char *)&g_outBuf.front(), g_outBuf.size(), wcOut, outLen);
      ui::setOutput(wcOut);
      delete[] wcOut;
    }
    ui::setState(ui::STATE_FINISH);
  } else {
    if (didOutput) {
      if (ui::outCharSet == IDM_OPT_OUTPUT_ASCII) {
        wchar_t wcOut[] = {0, 0};
        MultiByteToWideChar(CP_UTF8, 0, (char *)&output, 1, wcOut, 1);
        ui::appendOutput(wcOut);
      } else if (ui::outCharSet == IDM_OPT_OUTPUT_HEX) {
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
        g_outBuf.push_back(output);
      }
    }
    ui::setState(state);
  }

  return fin;
}

DWORD WINAPI threadRunner(LPVOID lpParameter) {
  UNREFERENCED_PARAMETER(lpParameter);

  while (g_ctrlThread == CTRLTHREAD_RUN && !bfNext(ui::hWnd, ui::STATE_RUN))
    ;

  PostMessageW(ui::hWnd, WM_APP_THREADEND, 0, 0);
  return 0;
}

// Executes the next code on each timer call.
static void CALLBACK timerRunner(UINT uTimerID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1,
                                 DWORD_PTR dw2) {
  UNREFERENCED_PARAMETER(uTimerID);
  UNREFERENCED_PARAMETER(uMsg);
  UNREFERENCED_PARAMETER(dwUser);
  UNREFERENCED_PARAMETER(dw1);
  UNREFERENCED_PARAMETER(dw2);

  // appendOutput(L"<TMR>"); // debug

  if (g_timerID && bfNext(ui::hWnd, ui::STATE_RUN)) {
    // appendOutput(L"<End>"); // debug
    timeKillEvent(g_timerID);
    timeEndPeriod(ui::speed);
    g_timerID = 0;
  }
}

static LRESULT CALLBACK wndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  static bool didInit = false;

  switch (uMsg) {
    case WM_CREATE:
      g_bf = new bf();
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

    case WM_PAINT:
      ui::onPaint();
      break;

    case WM_INITMENUPOPUP:
      ui::onInitMenuPopup();
      break;

    case WM_ACTIVATE:
      if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) ui::onActivate();
      break;

    case WM_APP_THREADEND:
      WaitForSingleObject(g_hThread, INFINITE);
      CloseHandle(g_hThread);
      g_hThread = NULL;
      if (g_ctrlThread == CTRLTHREAD_PAUSE) {
        ui::setState(ui::STATE_PAUSE);
      } else if (g_ctrlThread == CTRLTHREAD_END) {
        g_bf->reset();
        ui::setState(ui::STATE_INIT);
        didInit = false;
      }
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

          if (ui::speed == 0) {
            g_hThread = CreateThread(NULL, 0, threadRunner, NULL, 0, NULL);
            if (g_hThread) {
              SetThreadPriority(g_hThread, THREAD_PRIORITY_BELOW_NORMAL);
            } else {
              MessageBoxW(hWnd, L"Failed to create a runner thread.", L"Error", MB_ICONERROR);
            }
          } else {
            if (timeBeginPeriod(ui::speed) == TIMERR_NOERROR) {
              g_timerID = timeSetEvent(ui::speed, ui::speed, timerRunner, 0, TIME_PERIODIC);
              if (g_timerID == 0) {
                MessageBoxW(hWnd, L"This speed is not supported on your device. Try slowing down.",
                            L"Error", MB_ICONERROR);
              }
            } else {
              MessageBoxW(hWnd, L"This speed is not supported on your device. Try slowing down.",
                          L"Error", MB_ICONERROR);
            }
          }
          break;

        case IDC_CMDBTN_FIRST + 1:  // Next
          if (!didInit) {
            if (!bfInit()) {
              MessageBoxW(hWnd, L"Invalid input.", L"Error", MB_ICONWARNING);
              break;
            }
            didInit = true;
          }
          bfNext(hWnd, ui::STATE_PAUSE);
          break;

        case IDC_CMDBTN_FIRST + 2:  // Pause
          if (g_timerID) {
            timeKillEvent(g_timerID);
            timeEndPeriod(ui::speed);
            g_timerID = 0;
          }
          if (g_hThread) {
            g_ctrlThread = CTRLTHREAD_PAUSE;
          }
          ui::setState(ui::STATE_PAUSE);
          break;

        case IDC_CMDBTN_FIRST + 3:  // End
          if (g_timerID) {
            timeKillEvent(g_timerID);
            timeEndPeriod(ui::speed);
            g_timerID = 0;
          }
          if (g_hThread) {
            g_ctrlThread = CTRLTHREAD_END;
          } else {
            g_bf->reset();
            ui::setState(ui::STATE_INIT);
            didInit = false;
          }
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

        case IDM_OPT_MEMTYPE_SIGNED:
          ui::signedness = true;
          break;

        case IDM_OPT_MEMTYPE_UNSIGNED:
          ui::signedness = false;
          break;

        case IDM_OPT_OUTPUT_ASCII:
          ui::outCharSet = IDM_OPT_OUTPUT_ASCII;
          break;

        case IDM_OPT_OUTPUT_UTF8:
          ui::outCharSet = IDM_OPT_OUTPUT_UTF8;
          break;

        case IDM_OPT_OUTPUT_SJIS:
          ui::outCharSet = IDM_OPT_OUTPUT_SJIS;
          break;

        case IDM_OPT_OUTPUT_HEX:
          ui::outCharSet = IDM_OPT_OUTPUT_HEX;
          break;

        case IDM_OPT_INPUT_UTF8:
          ui::inCharSet = IDM_OPT_INPUT_UTF8;
          break;

        case IDM_OPT_INPUT_SJIS:
          ui::inCharSet = IDM_OPT_INPUT_SJIS;
          break;

        case IDM_OPT_INPUT_HEX:
          ui::inCharSet = IDM_OPT_INPUT_HEX;
          break;

        case IDM_OPT_NOINPUT_ERROR:
          ui::noInput = bf::NOINPUT_ERROR;
          break;

        case IDM_OPT_NOINPUT_ZERO:
          ui::noInput = bf::NOINPUT_ZERO;
          break;

        case IDM_OPT_NOINPUT_FF:
          ui::noInput = bf::NOINPUT_FF;
          break;

        case IDM_OPT_INTOVF_ERROR:
          ui::wrapInt = false;
          break;

        case IDM_OPT_INTOVF_WRAPAROUND:
          ui::wrapInt = true;
          break;

        case IDM_OPT_PTROVF_ERROR:
          ui::wrapPtr = false;
          break;

        case IDM_OPT_PTROVF_WRAPAROUND:
          ui::wrapPtr = true;
          break;

        case IDM_OPT_WORDWRAP:
          ui::switchWordwrap();
          break;

        case IDM_ABOUT:
          MessageBoxW(hWnd, APP_NAME L" version " APP_VERSION, L"About this software", 0);
          break;

        default:
          if (LOWORD(wParam) >= IDC_SCRKBD_FIRST && LOWORD(wParam) < IDC_SCRKBD_FIRST + 8) {
            ui::onScreenKeyboard(LOWORD(wParam) - IDC_SCRKBD_FIRST);
          }
      }
      if (LOWORD(wParam) != IDC_EDITOR && LOWORD(wParam) != IDC_INPUT &&
          LOWORD(wParam) != IDC_OUTPUT) {
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
  wcl.hbrBackground = NULL;
  if (!RegisterClassW(&wcl)) return FALSE;

  HWND hWnd = CreateWindowExW(0, WND_CLASS_NAME, APP_NAME, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                              CW_USEDEFAULT, CW_USEDEFAULT, 480, 320, NULL, NULL, hInstance, NULL);
  if (!hWnd) return FALSE;

  ShowWindow(hWnd, nShowCmd);
#ifdef UNDER_CE
  ShowWindow(hWnd, SW_MAXIMIZE);
#endif
  UpdateWindow(hWnd);

  MSG msg;
  while (GetMessageW(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  return (int)msg.wParam;
}
