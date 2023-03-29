#include <string>

#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#define UNICODE
#endif

// Since they conflict with the C++ STL and some SDKs don't have them,
// disables and defines them manually with different names.
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#define mymin(a, b) (((a) < (b)) ? (a) : (b))
#define mymax(a, b) (((a) > (b)) ? (a) : (b))

#ifdef UNDER_CE
#include <commctrl.h>
#include <commdlg.h>
#define adjust(coord) (coord)  // DPI scaling isn't needed for Windows CE.
#else
#define adjust(coord) ((coord)*dpi / 96)
// Since old SDKs don't have this enum, defines it manually.
typedef enum MONITOR_DPI_TYPE {
  MDT_EFFECTIVE_DPI = 0,
  MDT_ANGULAR_DPI = 1,
  MDT_RAW_DPI = 2,
  MDT_DEFAULT = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;

// Function pointer type for GetDpiForMonitor API.
typedef HRESULT(CALLBACK *GetDpiForMonitor_t)(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType,
                                              unsigned *dpiX, unsigned *dpiY);

// Function pointer type for TaskDialog API.
typedef HRESULT(__stdcall *TaskDialog_t)(HWND hwndOwner, HINSTANCE hInstance,
                                         const wchar_t *pszWindowTitle,
                                         const wchar_t *pszMainInstruction,
                                         const wchar_t *pszContent, int dwCommonButtons,
                                         const wchar_t *pszIcon, int *pnButton);
#endif

// Makes SetWindowLongW/GetWindowLongW compatible for both 32-bit and 64-bit system.
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

#include "bf.hpp"
#include "resource.h"
#include "ui.hpp"
#include "util.hpp"

namespace ui {
static const wchar_t *wcCmdBtn[CMDBTN_LEN] = {L"Run", L"Next", L"Pause", L"End"},
                     *wcScrKB[SCRKBD_LEN] = {L">", L"<", L"+", L"-", L".", L",", L"[", L"]", L"@"};
static HWND hEditor, hInput, hOutput, hMemView, hFocused, hCmdBtn[CMDBTN_LEN], hScrKB[SCRKBD_LEN];
static HMENU hMenu;
static HFONT hBtnFont = NULL, hEditFont = NULL;
static LOGFONTW editFont;
static int topPadding = 0, memViewStart = 0;
static wchar_t *retEditBuf = NULL, *retInBuf = NULL;
static std::wstring wstrFileName;
static bool withBOM = false, wordwrap = true;
static enum util::newline_t newLine = util::NEWLINE_CRLF;
#ifdef UNDER_CE
static HWND hCmdBar;
#else
static TaskDialog_t taskDialog = NULL;
static int dpi = 96, sysDPI = 96;
#endif

enum state_t state = STATE_INIT;
bool signedness = true, wrapInt = true, breakpoint = false, debug = true, dark = true,
     horizontal = false;
int speed = 10, outCharSet = IDM_BF_OUTPUT_UTF8, inCharSet = IDM_BF_INPUT_UTF8;
enum Brainfuck::noinput_t noInput = Brainfuck::NOINPUT_ZERO;
HWND hWnd;
HINSTANCE hInst;
HMODULE comctl32 = NULL;

// Enables/Disables menu items from the smaller nearest 10 multiple to `_endID`.
static void enableMenus(unsigned _endID, bool _enable) {
  unsigned i;
  for (i = (_endID / 10) * 10; i <= _endID; ++i) {
    EnableMenuItem(hMenu, i, MF_BYCOMMAND | (_enable ? MF_ENABLED : MF_GRAYED));
  }
}

void onCreate(HWND _hWnd, HINSTANCE _hInst) {
  size_t i;
  hWnd = _hWnd;
  hInst = _hInst;

#ifdef UNDER_CE
  wchar_t wcMenu[] = L"menu";  // CommandBar_InsertMenubarEx requires non-const value.
  InitCommonControls();
  hCmdBar = CommandBar_Create(hInst, hWnd, 1);
  CommandBar_InsertMenubarEx(hCmdBar, hInst, wcMenu, 0);
  CommandBar_Show(hCmdBar, TRUE);
  topPadding = CommandBar_Height(hCmdBar);
  hMenu = CommandBar_GetMenu(hCmdBar, 0);
#else
  hMenu = LoadMenu(hInst, L"menu");
  SetMenu(hWnd, hMenu);
#endif

  // Program editor
  hEditor =
      CreateWindowExW(0, L"EDIT", L"",
                      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                          WS_VSCROLL | (wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL) | ES_NOHIDESEL,
                      0, 0, 0, 0, hWnd, (HMENU)IDC_EDITOR, hInst, NULL);
  SendMessageW(hEditor, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  // Program input
  hInput =
      CreateWindowExW(0, L"EDIT", L"",
                      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                          WS_VSCROLL | (wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL) | ES_NOHIDESEL,
                      0, 0, 0, 0, hWnd, (HMENU)IDC_INPUT, hInst, NULL);
  SendMessageW(hInput, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  // Program output
  hOutput = CreateWindowExW(0, L"EDIT", L"",
                            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE |
                                ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL |
                                (wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL) | ES_NOHIDESEL,
                            0, 0, 0, 0, hWnd, (HMENU)IDC_OUTPUT, hInst, NULL);
  SendMessageW(hOutput, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  // Memory view
  hMemView = CreateWindowExW(0, L"EDIT", L"",
                             WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE |
                                 ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL | ES_NOHIDESEL,
                             0, 0, 0, 0, hWnd, (HMENU)IDC_MEMVIEW, hInst, NULL);
  SendMessageW(hMemView, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  // Command button
  for (i = 0; i < CMDBTN_LEN; ++i) {
    hCmdBtn[i] = CreateWindowExW(
        0, L"BUTTON", wcCmdBtn[i],
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | (i == 2 || i == 3 ? WS_DISABLED : 0), 0, 0, 0, 0,
        hWnd, (HMENU)(IDC_CMDBTN_FIRST + i), hInst, NULL);
  }

  // Screen keyboard
  for (i = 0; i < SCRKBD_LEN; ++i) {
    hScrKB[i] = CreateWindowExW(0, L"BUTTON", wcScrKB[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0,
                                0, 0, 0, hWnd, (HMENU)(IDC_SCRKBD_FIRST + i), hInst, NULL);
  }

  hFocused = hEditor;

  // Default configurations for the editor font.
  // We create the actual font object on onSize function.
  ZeroMemory(&editFont, sizeof(editFont));
  editFont.lfCharSet = DEFAULT_CHARSET;
  editFont.lfQuality = ANTIALIASED_QUALITY;
  editFont.lfHeight = -15;  // Represents font size 11 in 96 DPI.
#ifdef UNDER_CE
  // Sets a pre-installed font on Windows CE, as it doesn't have "MS Shell Dlg".
  lstrcpyW(editFont.lfFaceName, L"Tahoma");
#else
  // Sets a logical font face name for localization.
  // It maps to a default shell font associated with the current culture/locale.
  lstrcpyW(editFont.lfFaceName, L"MS Shell Dlg");

  // Obtains the "system DPI" value. We use this as the fallback value on older Windows versions
  // and to calculate the appropriate font height value for ChooseFontW.
  HDC hDC = GetDC(hWnd);
  sysDPI = GetDeviceCaps(hDC, LOGPIXELSX);
  ReleaseDC(hWnd, hDC);

  // Tries to load the GetDpiForMonitor API. Use of the full path improves security. We avoid a
  // direct call to keep this program compatible with Windows 7 and earlier.
  //
  // Microsoft recommends the use of GetDpiForWindow API instead of this API according to their
  // documentation. However, it requires Windows 10 1607 or later, which makes this compatibility
  // keeping code more complicated, and GetDpiForMonitor API still works for programs that only use
  // the process-wide DPI awareness. Here, as we only use the process-wide DPI awareness, we are
  // going to use GetDpiForMonitor API.
  //
  // References:
  // https://learn.microsoft.com/en-us/windows/win32/api/shellscalingapi/nf-shellscalingapi-getdpiformonitor
  // https://mariusbancila.ro/blog/2021/05/19/how-to-build-high-dpi-aware-native-desktop-applications/
  GetDpiForMonitor_t getDpiForMonitor = NULL;
  HMODULE dll = LoadLibraryW(L"C:\\Windows\\System32\\Shcore.dll");
  if (dll) {
    getDpiForMonitor = (GetDpiForMonitor_t)(void *)GetProcAddress(dll, "GetDpiForMonitor");
  }

  // Tests whether it successfully got the GetDpiForMonitor API.
  if (getDpiForMonitor) {  // It got (the system is presumably Windows 8.1 or later).
    unsigned tmpX, tmpY;
    getDpiForMonitor(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), MDT_EFFECTIVE_DPI, &tmpX,
                     &tmpY);
    dpi = tmpX;
  } else {  // It failed (the system is presumably older than Windows 8.1).
    dpi = sysDPI;
  }
  if (dll) FreeLibrary(dll);

  // Adjusts the windows size according to the DPI value.
  SetWindowPos(hWnd, NULL, 0, 0, adjust(480), adjust(320),
               SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

  // Tries to load the TaskDialog API which is a newer substitite of MessageBoxW.
  // This API is per monitor DPI aware but doesn't exist before Windows Vista.
  // To make the system select version 6 comctl32.dll, we don't use the full path here.
  comctl32 = LoadLibraryW(L"comctl32.dll");
  if (comctl32) {
    taskDialog = (TaskDialog_t)(void *)GetProcAddress(comctl32, "TaskDialog");
  }
#endif
}

void onDestroy() {
  DeleteObject(hBtnFont);
  DeleteObject(hEditFont);
  if (retEditBuf) delete[] retEditBuf;
  if (retInBuf) delete[] retInBuf;
  if (comctl32) FreeLibrary(comctl32);
}

void onSize() {
  RECT rect;

#ifdef UNDER_CE
  // Manually forces the minimum window size as Windows CE doesn't support WM_GETMINMAXINFO.
  // Seemingly, Windows CE doesn't re-send WM_SIZE on SetWindowPos calls inside a WM_SIZE handler.
  GetWindowRect(hWnd, &rect);
  if (rect.right - rect.left < 480 || rect.bottom - rect.top < 320) {
    SetWindowPos(hWnd, NULL, 0, 0, mymax(480, rect.right - rect.left),
                 mymax(320, rect.bottom - rect.top), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
  }
#endif

  int i;
  LOGFONTW rLogfont;

  // Button font
  ZeroMemory(&rLogfont, sizeof(rLogfont));
  rLogfont.lfHeight = adjust(-15);
  rLogfont.lfWeight = FW_BOLD;
  rLogfont.lfCharSet = DEFAULT_CHARSET;
  rLogfont.lfQuality = ANTIALIASED_QUALITY;
  lstrcpyW(rLogfont.lfFaceName, editFont.lfFaceName);  // Syncs with editors.
  HFONT newBtnFont = CreateFontIndirectW(&rLogfont);

  // Editor/Output font
  memcpy(&rLogfont, &editFont, sizeof(LOGFONTW));
  rLogfont.lfHeight = adjust(editFont.lfHeight);
  HFONT newEditFont = CreateFontIndirectW(&rLogfont);

#ifdef UNDER_CE
  // We must "move" the command bar to prevent a glitch.
  MoveWindow(hCmdBar, 0, 0, 0, 0, TRUE);
#endif

  // Moves and resizes controls, and applies the newly created fonts for them.
  int curX = 0;
  for (i = 0; i < CMDBTN_LEN; ++i) {
    MoveWindow(hCmdBtn[i], curX, topPadding, adjust(46), adjust(32), TRUE);
    SendMessageW(hCmdBtn[i], WM_SETFONT, (WPARAM)newBtnFont, MAKELPARAM(TRUE, 0));
    curX += adjust(46);
  }
  for (i = 0; i < SCRKBD_LEN; ++i) {
    MoveWindow(hScrKB[i], curX, topPadding, adjust(30), adjust(32), TRUE);
    SendMessageW(hScrKB[i], WM_SETFONT, (WPARAM)newBtnFont, MAKELPARAM(TRUE, 0));
    curX += adjust(30);
  }

  int topEditor = topPadding + adjust(32);
  GetClientRect(hWnd, &rect);
  int scrX = rect.right, scrY = rect.bottom;
  int halfY = (scrY - topEditor) / 2, centerY = (scrY + topEditor) / 2;
  if (horizontal) {
    MoveWindow(hEditor, 0, topEditor, scrX / 3, scrY - topEditor, TRUE);
    MoveWindow(hInput, scrX / 3, topEditor, scrX / 3, halfY, TRUE);
    MoveWindow(hOutput, scrX / 3, halfY + topEditor, scrX / 3, scrY - halfY - topEditor, TRUE);
    MoveWindow(hMemView, scrX * 2 / 3, topEditor, scrX - scrX * 2 / 3, scrY - topEditor, TRUE);
  } else {
    MoveWindow(hEditor, 0, topEditor, scrX, halfY, TRUE);
    MoveWindow(hInput, 0, centerY, scrX / 2, halfY / 2, TRUE);
    MoveWindow(hOutput, scrX / 2, centerY, scrX - scrX / 2, halfY / 2, TRUE);
    MoveWindow(hMemView, 0, centerY + halfY / 2, scrX, scrY - centerY - halfY / 2, TRUE);
  }
  SendMessageW(hEditor, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(TRUE, 0));
  SendMessageW(hInput, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(TRUE, 0));
  SendMessageW(hOutput, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(TRUE, 0));
  SendMessageW(hMemView, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(TRUE, 0));

  if (hBtnFont) DeleteObject(hBtnFont);
  if (hEditFont) DeleteObject(hEditFont);
  hBtnFont = newBtnFont;
  hEditFont = newEditFont;
  InvalidateRect(hWnd, NULL, FALSE);
}

void onInitMenuPopup() {
  // Brainfuck -> Memory Type
  CheckMenuRadioItem(hMenu, IDM_BF_MEMTYPE_SIGNED, IDM_BF_MEMTYPE_UNSIGNED,
                     signedness ? IDM_BF_MEMTYPE_SIGNED : IDM_BF_MEMTYPE_UNSIGNED, MF_BYCOMMAND);

  // Brainfuck -> Output Charset
  CheckMenuRadioItem(hMenu, IDM_BF_OUTPUT_UTF8, IDM_BF_OUTPUT_HEX, outCharSet, MF_BYCOMMAND);

  // Brainfuck -> Input Charset
  CheckMenuRadioItem(hMenu, IDM_BF_INPUT_UTF8, IDM_BF_INPUT_HEX, inCharSet, MF_BYCOMMAND);

  // Brainfuck -> Input Instruction
  CheckMenuRadioItem(hMenu, IDM_BF_NOINPUT_ERROR, IDM_BF_NOINPUT_FF, IDM_BF_NOINPUT_ERROR + noInput,
                     MF_BYCOMMAND);

  // Brainfuck -> Integer Overflow
  CheckMenuRadioItem(hMenu, IDM_BF_INTOVF_ERROR, IDM_BF_INTOVF_WRAPAROUND,
                     wrapInt ? IDM_BF_INTOVF_WRAPAROUND : IDM_BF_INTOVF_ERROR, MF_BYCOMMAND);

  // Brainfuck -> Breakpoint
  CheckMenuItem(hMenu, IDM_BF_BREAKPOINT, MF_BYCOMMAND | breakpoint ? MF_CHECKED : MF_UNCHECKED);

  // Options -> Speed
  if (speed == 0) {
    CheckMenuRadioItem(hMenu, IDM_OPT_SPEED_FASTEST, IDM_OPT_SPEED_100MS, IDM_OPT_SPEED_FASTEST,
                       MF_BYCOMMAND);
  } else if (speed == 1) {
    CheckMenuRadioItem(hMenu, IDM_OPT_SPEED_FASTEST, IDM_OPT_SPEED_100MS, IDM_OPT_SPEED_1MS,
                       MF_BYCOMMAND);
  } else if (speed == 10) {
    CheckMenuRadioItem(hMenu, IDM_OPT_SPEED_FASTEST, IDM_OPT_SPEED_100MS, IDM_OPT_SPEED_10MS,
                       MF_BYCOMMAND);
  } else if (speed == 100) {
    CheckMenuRadioItem(hMenu, IDM_OPT_SPEED_FASTEST, IDM_OPT_SPEED_100MS, IDM_OPT_SPEED_100MS,
                       MF_BYCOMMAND);
  }

  // Options -> Debug
  CheckMenuItem(hMenu, IDM_OPT_TRACK, MF_BYCOMMAND | debug ? MF_CHECKED : MF_UNCHECKED);

  // Options -> Word Wrap
  CheckMenuItem(hMenu, IDM_OPT_WORDWRAP, MF_BYCOMMAND | wordwrap ? MF_CHECKED : MF_UNCHECKED);

  bool undoable = SendMessageW(hEditor, EM_CANUNDO, 0, 0) != 0;

  if (state == STATE_INIT) {
    enableMenus(IDM_FILE_NEW, true);
    enableMenus(IDM_FILE_OPEN, true);
    enableMenus(IDM_EDIT_UNDO, undoable);
    enableMenus(IDM_BF_MEMTYPE_UNSIGNED, true);
    enableMenus(IDM_BF_OUTPUT_HEX, true);
    enableMenus(IDM_BF_INPUT_HEX, true);
    enableMenus(IDM_BF_NOINPUT_FF, true);
    enableMenus(IDM_BF_INTOVF_WRAPAROUND, true);
    enableMenus(IDM_BF_BREAKPOINT, true);
    enableMenus(IDM_OPT_SPEED_100MS, true);
    enableMenus(IDM_OPT_MEMVIEW, true);
    enableMenus(IDM_OPT_TRACK, true);
    enableMenus(IDM_OPT_HLTPROG, false);
    enableMenus(IDM_OPT_HLTMEM, false);
  } else if (state == STATE_RUN) {
    enableMenus(IDM_FILE_NEW, false);
    enableMenus(IDM_FILE_OPEN, false);
    enableMenus(IDM_EDIT_UNDO, false);
    enableMenus(IDM_BF_MEMTYPE_UNSIGNED, false);
    enableMenus(IDM_BF_OUTPUT_HEX, false);
    enableMenus(IDM_BF_INPUT_HEX, false);
    enableMenus(IDM_BF_NOINPUT_FF, false);
    enableMenus(IDM_BF_INTOVF_WRAPAROUND, false);
    enableMenus(IDM_BF_BREAKPOINT, false);
    enableMenus(IDM_OPT_SPEED_100MS, false);
    enableMenus(IDM_OPT_MEMVIEW, false);
    enableMenus(IDM_OPT_TRACK, false);
    enableMenus(IDM_OPT_HLTPROG, false);
    enableMenus(IDM_OPT_HLTMEM, false);
  } else if (state == STATE_PAUSE || state == STATE_FINISH) {
    enableMenus(IDM_FILE_NEW, false);
    enableMenus(IDM_FILE_OPEN, false);
    enableMenus(IDM_EDIT_UNDO, false);
    enableMenus(IDM_BF_MEMTYPE_UNSIGNED, false);
    enableMenus(IDM_BF_OUTPUT_HEX, false);
    enableMenus(IDM_BF_INPUT_HEX, false);
    enableMenus(IDM_BF_NOINPUT_FF, false);
    enableMenus(IDM_BF_INTOVF_WRAPAROUND, false);
    enableMenus(IDM_BF_BREAKPOINT, true);
    enableMenus(IDM_OPT_SPEED_100MS, true);
    enableMenus(IDM_OPT_MEMVIEW, true);
    enableMenus(IDM_OPT_TRACK, true);
    enableMenus(IDM_OPT_HLTPROG, true);
    enableMenus(IDM_OPT_HLTMEM, true);
  }
}

#ifndef UNDER_CE
void onDropFiles(HDROP hDrop) {
  wchar_t wcFileName[MAX_PATH];
  DragQueryFileW(hDrop, 0, wcFileName, MAX_PATH);
  DragFinish(hDrop);
  openFile(false, wcFileName);
}

void onGetMinMaxInfo(MINMAXINFO *_minMaxInfo) {
  _minMaxInfo->ptMinTrackSize.x = adjust(480);
  _minMaxInfo->ptMinTrackSize.y = adjust(320);
}

void onDPIChanged(int _dpi, const RECT *_rect) {
  dpi = _dpi;
  MoveWindow(hWnd, _rect->left, _rect->top, _rect->right - _rect->left, _rect->bottom - _rect->top,
             FALSE);
}
#endif

void onScreenKeyboard(int _key) {
  if (hFocused == hEditor) SendMessageW(hEditor, EM_REPLACESEL, 0, (WPARAM)wcScrKB[_key]);
}

void cut() { SendMessageW(hFocused, WM_CUT, 0, 0); }

void copy() { SendMessageW(hFocused, WM_COPY, 0, 0); }

void paste() { SendMessageW(hFocused, WM_PASTE, 0, 0); }

void selAll() { SendMessageW(hFocused, EM_SETSEL, 0, -1); }

void undo() { SendMessageW(hEditor, EM_UNDO, 0, 0); }

int messageBox(HWND _hWnd, const wchar_t *_lpText, const wchar_t *_lpCaption, unsigned _uType) {
#ifndef UNDER_CE
  if (taskDialog) {
    // Tests whether _uType uses some features that TaskDialog doesn't support.
    if (_uType & ~(MB_ICONMASK | MB_TYPEMASK)) goto mbfallback;

    int buttons;
    switch (_uType & MB_TYPEMASK) {
      case MB_OK:
        buttons = 1;
        break;
      case MB_OKCANCEL:
        buttons = 1 + 8;
        break;
      case MB_RETRYCANCEL:
        buttons = 16 + 8;
        break;
      case MB_YESNO:
        buttons = 2 + 4;
        break;
      case MB_YESNOCANCEL:
        buttons = 2 + 4 + 8;
        break;
      default:  // Not supported by TaskDialog.
        goto mbfallback;
    }

    wchar_t *icon;
    switch (_uType & MB_ICONMASK) {
      case 0:
        icon = NULL;
        break;
      case MB_ICONWARNING:  // Same value as MB_ICONEXCLAMATION.
        icon = MAKEINTRESOURCEW(-1);
        break;
      case MB_ICONERROR:  // Same value as MB_ICONSTOP and MB_ICONHAND.
        icon = MAKEINTRESOURCEW(-2);
        break;
      default:  // Fallbacks everything else for Information icon.
        icon = MAKEINTRESOURCEW(-3);
    }

    int result;
    taskDialog(_hWnd, hInst, _lpCaption, L"", _lpText, buttons, icon, &result);
    return result;
  }
mbfallback:
#endif
  return MessageBoxW(_hWnd, _lpText, _lpCaption, _uType);
}

// Hook window procedure for the edit control in the memory view options dialog.
// This procedure translates top row character keys to numbers according to the keyboard layout of
// SHARP Brain.
static LRESULT CALLBACK memViewDlgEditor(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam) {
  static WNDPROC prevWndProc = (WNDPROC)myGetWindowLongW(hWnd, GWL_USERDATA);

  switch (uMsg) {
    case WM_CHAR:
      switch ((wchar_t)wParam) {
        case L'q':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"1");
          return 0;

        case L'w':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"2");
          return 0;

        case L'e':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"3");
          return 0;

        case L'r':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"4");
          return 0;

        case L't':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"5");
          return 0;

        case L'y':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"6");
          return 0;

        case L'u':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"7");
          return 0;

        case L'i':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"8");
          return 0;

        case L'o':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"9");
          return 0;

        case L'p':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"0");
          return 0;
      }
      break;

    case WM_DESTROY:
      mySetWindowLongW(hWnd, GWL_WNDPROC, prevWndProc);
      return 0;
  }

  return CallWindowProcW(prevWndProc, hWnd, uMsg, wParam, lParam);
}

// Window procedure for the memory view options dialog.
INT_PTR CALLBACK memViewProc(HWND hDlg, unsigned uMsg, WPARAM wParam, LPARAM lParam) {
  UNREFERENCED_PARAMETER(lParam);

  switch (uMsg) {
    case WM_INITDIALOG: {
      // Puts the dialog at the center of the parent window.
      RECT wndSize, wndRect, dlgRect;
      GetWindowRect(hDlg, &dlgRect);
      GetWindowRect(hWnd, &wndRect);
      GetClientRect(hWnd, &wndSize);
      int newPosX = wndRect.left + (wndSize.right - (dlgRect.right - dlgRect.left)) / 2,
          newPosY = wndRect.top + (wndSize.bottom - (dlgRect.bottom - dlgRect.top)) / 2;
      if (newPosX < 0) newPosX = 0;
      if (newPosY < 0) newPosY = 0;
      SetWindowPos(hDlg, NULL, newPosX, newPosY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

      wchar_t editBuf[10];
      HWND hEdit = GetDlgItem(hDlg, 3);
      wsprintfW(editBuf, L"%u", memViewStart <= 999999999 ? memViewStart : 999999999);
      SendDlgItemMessageW(hDlg, 3, EM_SETLIMITTEXT, 9, 0);
      SetDlgItemTextW(hDlg, 3, editBuf);
      mySetWindowLongW(hEdit, GWL_USERDATA, mySetWindowLongW(hEdit, GWL_WNDPROC, memViewDlgEditor));
      return TRUE;
    }

    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDOK: {
          wchar_t editBuf[10];
          long temp;
          GetDlgItemTextW(hDlg, 3, (wchar_t *)editBuf, 10);
          temp = wcstol(editBuf, NULL, 10);
          if (temp < 0) {
            messageBox(hDlg, L"Invalid input.", L"Error", MB_ICONWARNING);
          } else {
            memViewStart = temp;
            EndDialog(hDlg, IDOK);
          }
          return TRUE;
        }

        case IDCANCEL:
          EndDialog(hDlg, IDCANCEL);
          return TRUE;
      }
      return FALSE;
  }
  return FALSE;
}

void setState(enum state_t _state, bool _force) {
  int i;
  if (!_force && _state == state) return;

  if (_state == STATE_INIT) {
    EnableWindow(hCmdBtn[0], TRUE);   // run button
    EnableWindow(hCmdBtn[1], TRUE);   // next button
    EnableWindow(hCmdBtn[2], FALSE);  // pause button
    EnableWindow(hCmdBtn[3], FALSE);  // end button
    SendMessageW(hEditor, EM_SETREADONLY, (WPARAM)FALSE, (LPARAM)NULL);
    SendMessageW(hInput, EM_SETREADONLY, (WPARAM)FALSE, (LPARAM)NULL);
    SendMessageW(hOutput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    for (i = 0; i < SCRKBD_LEN; ++i) {
      EnableWindow(hScrKB[i], TRUE);
    }
  } else if (_state == STATE_RUN) {
    EnableWindow(hCmdBtn[0], FALSE);  // run button
    EnableWindow(hCmdBtn[1], FALSE);  // next button
    EnableWindow(hCmdBtn[2], TRUE);   // pause button
    EnableWindow(hCmdBtn[3], TRUE);   // end button
    SendMessageW(hEditor, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(hInput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(hOutput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    for (i = 0; i < SCRKBD_LEN; ++i) {
      EnableWindow(hScrKB[i], FALSE);
    }
  } else if (_state == STATE_PAUSE) {
    EnableWindow(hCmdBtn[0], TRUE);   // run button
    EnableWindow(hCmdBtn[1], TRUE);   // next button
    EnableWindow(hCmdBtn[2], FALSE);  // pause button
    EnableWindow(hCmdBtn[3], TRUE);   // end button
    SendMessageW(hEditor, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(hInput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(hOutput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    for (i = 0; i < SCRKBD_LEN; ++i) {
      EnableWindow(hScrKB[i], FALSE);
    }
  } else if (_state == STATE_FINISH) {
    EnableWindow(hCmdBtn[0], FALSE);  // run button
    EnableWindow(hCmdBtn[1], FALSE);  // next button
    EnableWindow(hCmdBtn[2], FALSE);  // pause button
    EnableWindow(hCmdBtn[3], TRUE);   // end button
    SendMessageW(hEditor, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(hInput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(hOutput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    for (i = 0; i < SCRKBD_LEN; ++i) {
      EnableWindow(hScrKB[i], FALSE);
    }
  }

  state = _state;
  InvalidateRect(hWnd, NULL, FALSE);
}

void updateFocus(int _id) {
  switch (_id) {
    case IDC_EDITOR:
      hFocused = hEditor;
      break;

    case IDC_INPUT:
      hFocused = hInput;
      break;

    case IDC_OUTPUT:
      hFocused = hOutput;
      break;

    case IDC_MEMVIEW:
      hFocused = hMemView;
      break;

    default:
      SetFocus(hFocused);
  }
}

void selProg(unsigned _progPtr) { SendMessageW(hEditor, EM_SETSEL, _progPtr, _progPtr + 1); }

void selMemView(unsigned _memPtr) {
  SendMessageW(hMemView, EM_SETSEL, (_memPtr - memViewStart) * 3, (_memPtr - memViewStart) * 3 + 2);
}

void setMemory(const unsigned char *memory, int size) {
  if (!memory) {
    SetWindowTextW(hMemView, NULL);
    return;
  }

  int i;
  std::wstring wstrOut;
  for (i = memViewStart; i < memViewStart + 100 && i < size; ++i) {
    wchar_t wcOut[4];
    util::toHex(memory[i], wcOut);
    wstrOut.append(wcOut);
  }
  SetWindowTextW(hMemView, wstrOut.c_str());
}

wchar_t *getEditor() {
  if (retEditBuf) free(retEditBuf);

  int editorSize = GetWindowTextLengthW(hEditor) + 1;
  if ((retEditBuf = (wchar_t *)malloc(sizeof(wchar_t) * editorSize))) {
    GetWindowTextW(hEditor, retEditBuf, editorSize);
  } else {
    messageBox(hWnd, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
  }

  return retEditBuf;
}

wchar_t *getInput() {
  if (retInBuf) free(retInBuf);

  int inputSize = GetWindowTextLengthW(hInput) + 1;
  if ((retInBuf = (wchar_t *)malloc(sizeof(wchar_t) * inputSize))) {
    GetWindowTextW(hInput, retInBuf, inputSize);
  } else {
    messageBox(hWnd, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
  }

  return retInBuf;
}

void setOutput(const wchar_t *_str) { SetWindowTextW(hOutput, _str); }

void appendOutput(const wchar_t *_str) {
  int editLen = (int)SendMessageW(hOutput, WM_GETTEXTLENGTH, 0, 0);
  SendMessageW(hOutput, EM_SETSEL, editLen, editLen);
  SendMessageW(hOutput, EM_REPLACESEL, 0, (WPARAM)_str);
}

void switchWordwrap() {
  int editorSize = GetWindowTextLengthW(hEditor) + 1;
  wchar_t *wcEditor = (wchar_t *)malloc(sizeof(wchar_t) * editorSize);
  if (!wcEditor) {
    messageBox(hWnd, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return;
  }
  GetWindowTextW(hEditor, wcEditor, editorSize);
  bool isModified = SendMessageW(hEditor, EM_GETMODIFY, 0, 0) != 0;
  DestroyWindow(hEditor);

  int inputSize = GetWindowTextLengthW(hInput) + 1;
  wchar_t *wcInput = (wchar_t *)malloc(sizeof(wchar_t) * inputSize);
  if (!wcInput) {
    free(wcEditor);
    messageBox(hWnd, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return;
  }
  GetWindowTextW(hInput, wcInput, inputSize);
  DestroyWindow(hInput);

  int outputSize = GetWindowTextLengthW(hOutput) + 1;
  wchar_t *wcOutput = (wchar_t *)malloc(sizeof(wchar_t) * outputSize);
  if (!wcOutput) {
    free(wcEditor);
    free(wcInput);
    messageBox(hWnd, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return;
  }
  GetWindowTextW(hOutput, wcOutput, outputSize);
  DestroyWindow(hOutput);

  wordwrap = !wordwrap;

  // Program editor
  hEditor =
      CreateWindowExW(0, L"EDIT", L"",
                      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                          WS_VSCROLL | (wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL) | ES_NOHIDESEL,
                      0, 0, 0, 0, hWnd, (HMENU)IDC_EDITOR, hInst, NULL);
  SendMessageW(hInput, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  // Program input
  hInput =
      CreateWindowExW(0, L"EDIT", L"",
                      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                          WS_VSCROLL | (wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL) | ES_NOHIDESEL,
                      0, 0, 0, 0, hWnd, (HMENU)IDC_INPUT, hInst, NULL);
  SendMessageW(hInput, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  // Program output
  hOutput = CreateWindowExW(0, L"EDIT", L"",
                            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE |
                                ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL |
                                (wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL) | ES_NOHIDESEL,
                            0, 0, 0, 0, hWnd, (HMENU)IDC_OUTPUT, hInst, NULL);
  SendMessageW(hOutput, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  hFocused = hEditor;
  updateFocus();
  onSize();
  setState(state, true);

  SetWindowTextW(hEditor, wcEditor);
  SendMessageW(hEditor, EM_SETMODIFY, isModified, 0);
  SetWindowTextW(hInput, wcInput);
  SetWindowTextW(hOutput, wcOutput);

  free(wcEditor);
  free(wcInput);
  free(wcOutput);
}

void switchTheme() {
  dark = !dark;

  InvalidateRect(hEditor, NULL, TRUE);
  InvalidateRect(hInput, NULL, TRUE);
  InvalidateRect(hOutput, NULL, TRUE);
  InvalidateRect(hMemView, NULL, TRUE);
}

void switchLayout() {
  horizontal = !horizontal;

  onSize();
}

void chooseFont() {
#ifndef UNDER_CE
  LONG origHeight = editFont.lfHeight;
  // Temporarilly sets to the "System DPI scaled" value since ChooseFontW expects it.
  // Subtracting by 96 - 1 makes this division to behave like a ceiling function.
  // Note that editFont.lfHeight is negative.
  editFont.lfHeight = (editFont.lfHeight * sysDPI - (96 - 1)) / 96;
#endif

  CHOOSEFONTW cf;
  cf.lStructSize = sizeof(CHOOSEFONTW);
  cf.hwndOwner = hWnd;
  cf.lpLogFont = &editFont;
  cf.hDC = GetDC(hWnd);  // Required for CF_BOTH.
  // We won't get any fonts if we omit CF_BOTH on Windows CE.
  cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST | CF_BOTH;
  BOOL ret = ChooseFontW(&cf);
  ReleaseDC(hWnd, cf.hDC);

#ifdef UNDER_CE
  if (ret) {
    editFont.lfQuality = ANTIALIASED_QUALITY;
    onSize();
  }
#else
  if (ret) {
    // Re-converts to the 96 DPI value as we properly adjust it on the fly.
    editFont.lfHeight = (editFont.lfHeight * 96 - (sysDPI - 1)) / sysDPI;
    editFont.lfQuality = ANTIALIASED_QUALITY;
    onSize();
  } else {
    // Rewrites the original value instead of re-converting.
    // Re-converting can results in a different value.
    editFont.lfHeight = origHeight;
  }
#endif
}

bool promptSave() {
  if (SendMessageW(hEditor, EM_GETMODIFY, 0, 0) == 0) return true;

  int ret = messageBox(hWnd, L"Unsaved data will be lost. Save changes?", L"Confirm",
                       MB_ICONWARNING | MB_YESNOCANCEL);

  if (ret == IDCANCEL) {
    return false;
  } else if (ret == IDYES) {
    if (saveFile(true)) {
      return true;
    } else {
      return false;
    }
  } else if (ret == IDNO) {
    return true;
  }

  // Shouldn't be reached.
  return true;
}

void openFile(bool _newFile, const wchar_t *_fileName) {
  wchar_t wcFileName[MAX_PATH] = {0};

  if (!promptSave()) return;

  if (_newFile) {  // new
    SetWindowTextW(hEditor, L"");
    SetWindowTextW(hWnd, APP_NAME);
    SendMessageW(hEditor, EM_SETMODIFY, FALSE, 0);
    wstrFileName = L"";
    withBOM = false;
    newLine = util::NEWLINE_CRLF;
    return;
  }

  if (!_fileName) {
    wchar_t initDir[MAX_PATH] = {0};
    if (!wstrFileName.empty()) {
      wstrFileName.substr(0, wstrFileName.rfind(L'\\')).copy(initDir, MAX_PATH - 1);
    }
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"Brainfuck source (*.bf;*.b;*.txt)\0*.bf;*.b;*.txt\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = wcFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L".bf";
    ofn.lpstrInitialDir = initDir[0] ? initDir : NULL;
    ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;
    _fileName = wcFileName;
  }

  HANDLE hFile = CreateFileW(_fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    messageBox(hWnd, L"Open failed.", L"Error", MB_ICONWARNING);
    return;
  }

  DWORD fileSize = GetFileSize(hFile, NULL), readLen;
  if (fileSize >= 65536 * 2) {
    messageBox(hWnd, L"This file is too large.", L"Error", MB_ICONWARNING);
    return;
  }

  char *fileBuf = (char *)malloc(sizeof(char) * fileSize);
  if (!fileBuf) {
    messageBox(hWnd, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return;
  }

  ReadFile(hFile, fileBuf, fileSize, &readLen, NULL);
  CloseHandle(hFile);

  int padding = 0;
  if (fileBuf[0] == '\xEF' && fileBuf[1] == '\xBB' && fileBuf[2] == '\xBF') padding = 3;  // BOM

  int length = MultiByteToWideChar(CP_UTF8, 0, fileBuf + padding, readLen - padding, NULL, 0);
  wchar_t *wcFileBuf = (wchar_t *)calloc(length + 1, sizeof(wchar_t));
  if (!wcFileBuf) {
    free(fileBuf);
    messageBox(hWnd, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return;
  }

  MultiByteToWideChar(CP_UTF8, 0, fileBuf + padding, readLen - padding, wcFileBuf, length);
  free(fileBuf);

  std::wstring converted = wcFileBuf;
  free(wcFileBuf);
  newLine = util::convertCRLF(converted, util::NEWLINE_CRLF);

  SetWindowTextW(hEditor, converted.c_str());
  SendMessageW(hEditor, EM_SETMODIFY, FALSE, 0);
  wstrFileName = _fileName;
  withBOM = padding != 0;

  std::wstring title = L"[";
  title.append(wstrFileName.substr(wstrFileName.rfind(L'\\') + 1) + L"] - " APP_NAME);
  SetWindowTextW(hWnd, title.c_str());
}

bool saveFile(bool _isOverwrite) {
  wchar_t wcFileName[MAX_PATH] = {0};

  if (!_isOverwrite || wstrFileName.empty()) {
    wchar_t initDir[MAX_PATH] = {0};
    if (!wstrFileName.empty()) {
      wstrFileName.substr(wstrFileName.rfind(L'\\') + 1).copy(wcFileName, MAX_PATH - 1);
      wstrFileName.substr(0, wstrFileName.rfind(L'\\')).copy(initDir, MAX_PATH - 1);
    }
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFilter = L"Brainfuck source (*.bf;*.b;*.txt)\0*.bf;*.b;*.txt\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = wcFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L".bf";
    ofn.lpstrInitialDir = initDir[0] ? initDir : NULL;
    ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    if (!GetSaveFileNameW(&ofn)) return false;
  } else {
    wstrFileName.copy(wcFileName, MAX_PATH);
  }

  int editorSize = GetWindowTextLengthW(hEditor) + 1;
  wchar_t *wcEditor = (wchar_t *)malloc(sizeof(wchar_t) * editorSize);
  if (!wcEditor) {
    messageBox(hWnd, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return false;
  }
  GetWindowTextW(hEditor, wcEditor, editorSize);
  std::wstring converted = wcEditor;
  free(wcEditor);

  if (newLine != util::NEWLINE_CRLF) util::convertCRLF(converted, newLine);
  int length = WideCharToMultiByte(CP_UTF8, 0, converted.c_str(), -1, NULL, 0, NULL, NULL);
  char *szEditor = (char *)malloc(sizeof(char) * length);
  if (!szEditor) {
    messageBox(hWnd, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return false;
  }
  WideCharToMultiByte(CP_UTF8, 0, converted.c_str(), -1, szEditor, length, NULL, NULL);

  HANDLE hFile = CreateFileW(wcFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    messageBox(hWnd, L"Open failed.", L"Error", MB_ICONWARNING);
    return false;
  }

  DWORD dwTemp;
  if (withBOM) WriteFile(hFile, "\xEF\xBB\xBF", 3, &dwTemp, NULL);  // BOM
  WriteFile(hFile, szEditor, length - 1, &dwTemp, NULL);
  CloseHandle(hFile);
  free(szEditor);

  SendMessageW(hEditor, EM_SETMODIFY, FALSE, 0);
  wstrFileName = wcFileName;

  std::wstring title = L"[";
  title.append(wstrFileName.substr(wstrFileName.rfind(L'\\') + 1) + L"] - " APP_NAME);
  SetWindowTextW(hWnd, title.c_str());

  return true;
}
}  // namespace ui
