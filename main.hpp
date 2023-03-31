#ifndef MAIN_HPP_
#define MAIN_HPP_

#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#define UNICODE
#endif

// Microsoft provided min/max macros conflict with C++ STL and even some Windows SDKs lack them.
// So we disable them here and redefine ourself.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define mymin(a, b) (((a) < (b)) ? (a) : (b))
#define mymax(a, b) (((a) > (b)) ? (a) : (b))

#include <windows.h>

#ifdef UNDER_CE
#include <commctrl.h>
#include <commdlg.h>
#else
#include <process.h>
#endif

#include <string>

#include "bf.hpp"
#include "resource.h"
#include "util.hpp"
#include "history.hpp"

// Workaround for wrong macro definitions in CeGCC.
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

// Expands to _beginthreadex() on Windows PC and CreateThread() on Windows CE.
#define myCreateThread(lpsa, cbStack, lpStartAddr, lpvThreadParam, fdwCreate, lpIDThread) \
  CreateThread(lpsa, cbStack, lpStartAddr, lpvThreadParam, fdwCreate, lpIDThread)

// WinMain is already unicode on Windows CE.
#define wWinMain WinMain
#else
// Expands to _beginthreadex() on Windows PC and CreateThread() on Windows CE.
#define myCreateThread(lpsa, cbStack, lpStartAddr, lpvThreadParam, fdwCreate, lpIDThread) \
  (HANDLE) _beginthreadex(lpsa, cbStack, lpStartAddr, lpvThreadParam, fdwCreate, lpIDThread)

// We define this enum manually as old SDKs don't have it.
typedef enum MONITOR_DPI_TYPE {
  MDT_EFFECTIVE_DPI = 0,
  MDT_ANGULAR_DPI = 1,
  MDT_RAW_DPI = 2,
  MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;

// The function pointer type for GetDpiForMonitor API.
typedef HRESULT(CALLBACK *GetDpiForMonitor_t)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType,
                                              unsigned *dpiX, unsigned *dpiY);
#endif

// Making SetWindowLongW and GetWindowLongW compatible for both 32-bit and 64-bit system.
#ifdef _WIN64
#define mySetWindowLongW(hWnd, index, data) SetWindowLongPtrW(hWnd, index, (LRESULT)(data))
#define myGetWindowLongW(hWnd, index) GetWindowLongPtrW(hWnd, index)
#ifndef GWL_WNDPROC
#define GWL_WNDPROC GWLP_WNDPROC
#endif
#ifndef GWL_USERDATA
#define GWL_USERDATA GWLP_USERDATA
#endif
#else
#define mySetWindowLongW(hWnd, index, data) SetWindowLongW(hWnd, index, (LONG)(data))
#define myGetWindowLongW(hWnd, index) GetWindowLongW(hWnd, index)
#endif

#ifndef WS_OVERLAPPEDWINDOW
#define WS_OVERLAPPEDWINDOW \
  WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
#endif
#define WM_APP_THREADEND WM_APP

namespace global {
enum state_t { STATE_INIT, STATE_RUN, STATE_PAUSE, STATE_FINISH };

extern HINSTANCE hInst;
extern HWND hWnd;
extern HWND hEditor;
extern HWND hInput;
extern HWND hOutput;
extern HWND hMemView;
extern HWND hFocused;
extern HWND hCmdBtn[CMDBTN_LEN];
extern HWND hScrKB[SCRKBD_LEN];
extern HMENU hMenu;
extern LOGFONTW editFont;
extern int speed;
extern int outCharSet;
extern int inCharSet;
extern int memViewStart;
extern bool signedness;
extern bool wrapInt;
extern bool breakpoint;
extern bool debug;
extern bool dark;
extern bool horizontal;
extern bool withBOM;
extern bool wordwrap;
extern wchar_t *cmdLine;
extern std::wstring wstrFileName;
extern class History history;
extern class History inputHistory;
extern Brainfuck::noinput_t noInput;
extern enum util::newline_t newLine;
extern enum state_t state;
#ifndef UNDER_CE
extern int sysDPI;
#endif
}  // namespace global

#endif
