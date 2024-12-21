#include "main.hpp"

#include "msg.hpp"
#include "runner.hpp"
#include "wproc.hpp"

#define WND_CLASS_NAME L"brainfuck-main"

namespace global {
HINSTANCE hInst;                                              // Handle to the current instance.
HWND hWnd;                                                    // Handle to the main window.
HWND hEditor;                                                 // Handle to the program editor.
HWND hInput;                                                  // Handle to the stdin editor.
HWND hOutput;                                                 // Handle to the stdout editor.
HWND hMemView;                                                // Handle to the memory view.
HWND hFocused;                                                // Handle to the currently focused control.
HWND hCmdBtn[CMDBTN_LEN];                                     // Handles to command buttons.
HWND hScrKB[SCRKBD_LEN];                                      // Handles to screen keyboard buttons.
HMENU hMenu;                                                  // Handle to the menu bar.
LOGFONTW editFont;                                            // Font information which is used in whole this app.
int speed = 10;                                               // Brainfuck execution speed.
int outCharSet = IDM_BF_OUTPUT_UTF8;                          // Brainfuck stdout charset.
int inCharSet = IDM_BF_INPUT_UTF8;                            // Brainfuck stdin charset.
int memViewStart = 0;                                         // First address of the memory view (0-indexed byte).
bool signedness = true;                                       // Signedness of the Brainfuck memory. True for signed.
bool wrapInt = true;                                          // Whether to wrap around an integer in Brainfuck.
bool breakpoint = false;                                      // Whether to enable breakpoints in Brainfuck.
bool debug = true;                                            // Whether to enable the real time debugging.
bool dark = true;                                             // Theme for this app. True for dark.
bool horizontal = false;                                      // Layout for this app.
bool withBOM = false;                                         // Whether the opened file contained an UTF-8 BOM.
bool wordwrap = true;                                         // Whether to enable the word wrap on editors.
wchar_t *cmdLine;                                             // Pointer to the command line.
std::wstring wstrFileName;                                    // Current file name.
class History history;                                        // Program undo/redo history.
class History inputHistory;                                   // Input undo/redo history.
enum Brainfuck::noinput_t noInput = Brainfuck::NOINPUT_ZERO;  // Brainfuck behavior on no input.
enum util::newline_t newLine = util::NEWLINE_CRLF;            // Newline code for the current file.
enum state_t state = STATE_INIT;                              // Current state of this app.
#ifndef UNDER_CE
int sysDPI = 96;  // The value of system DPI.
#endif
}  // namespace global

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t *lpCmdLine, int nShowCmd) {
  int i;
  UNREFERENCED_PARAMETER(hPrevInstance);

  SetDllDirectoryW(L"");  // DLL hijacking prevention.
  global::hInst = hInstance;

  while (*lpCmdLine == L' ') ++lpCmdLine;
  bool isQuoted = false;
  if (*lpCmdLine == L'"') {
    isQuoted = true;
    ++lpCmdLine;
  }
  for (i = 0; lpCmdLine[i]; ++i) {  // Replaces slashes with backslashes, and removes a closing quote.
    if (lpCmdLine[i] == L'"' || (lpCmdLine[i] == L' ' && !isQuoted)) {
      lpCmdLine[i] = L'\0';
      break;
    }
    if (lpCmdLine[i] == L'/') lpCmdLine[i] = L'\\';
  }
  for (--i; i >= 0 && lpCmdLine[i] == ' '; --i) lpCmdLine[i] = L'\0';  // Removes trailing spaces.
  global::cmdLine = lpCmdLine;

  WNDCLASSW wcl;
  memset(&wcl, 0, sizeof(WNDCLASSW));
  wcl.hInstance = hInstance;
  wcl.lpszClassName = WND_CLASS_NAME;
  wcl.lpfnWndProc = wproc::wndProc;
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

  HWND hWnd = CreateWindowExW(0, WND_CLASS_NAME, APP_NAME, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT,
                              CW_USEDEFAULT, 480, 320, NULL, NULL, hInstance, NULL);
  if (!hWnd) return FALSE;

  ShowWindow(hWnd, nShowCmd);
#ifdef UNDER_CE
  ShowWindow(hWnd, SW_MAXIMIZE);
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
