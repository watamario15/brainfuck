#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#include <windows.h>
#define mymin(a, b) (((a) < (b)) ? (a) : (b))
#define adjustX(x) ((x)*scrX / 480)
#define adjustY(y) ((y)*scrY / 320)

#ifdef UNDER_CE
#include <commctrl.h>
#include <commdlg.h>
#endif

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

#include <string>

#include "bf.hpp"
#include "def.h"
#include "ui.hpp"

namespace ui {
static const wchar_t *wcCmdBtn[] = {L"Run", L"Next", L"Pause", L"End"},
                     *wcScrKB[] = {L">", L"<", L"+", L"-", L".", L",", L"[", L"]", L"@"};
static HWND hEditor, hInput, hOutput, hMemView, hCmdBtn[sizeof(wcCmdBtn) / sizeof(wcCmdBtn[0])],
    hScrKB[sizeof(wcScrKB) / sizeof(wcScrKB[0])];
static HMENU hMenu;
static HFONT hBtnFont = NULL, hEditFont = NULL;
static int topPadding = 0, scrX = 480, scrY = 320, memCache[100];
static unsigned int memViewStart = 0;
static wchar_t *retEditBuf = NULL, *retInBuf = NULL;
static std::wstring wstrFileName;
static bool withBOM = false, wordwrap = false;
#ifdef UNDER_CE
static HWND hCmdBar;
#endif

enum state_t state = STATE_INIT;
bool signedness = true, wrapInt = true, debug = false;
int speed = 10, outCharSet = IDM_OPT_OUTPUT_ASCII, inCharSet = IDM_OPT_INPUT_UTF8;
enum Brainfuck::noinput_t noInput = Brainfuck::NOINPUT_ZERO;
HWND hWnd;
HINSTANCE hInst;

// Switches between enabled/disabled of a submenu.
// Expects the menu is starting from the smaller nearest 10 multiple.
static void enableAllSubMenu(unsigned int _endID, bool _enable) {
  unsigned int i;
  for (i = (_endID / 10) * 10; i <= _endID; ++i) {
    EnableMenuItem(hMenu, i, MF_BYCOMMAND | (_enable ? MF_ENABLED : MF_GRAYED));
  }
}

void onCreate(HWND _hWnd, HINSTANCE _hInst) {
  size_t i;
  hWnd = _hWnd;
  hInst = _hInst;

#ifdef UNDER_CE
  wchar_t wcMenu[] = L"Menu";  // CommandBar_InsertMenubarEx requires non-const value
  InitCommonControls();
  hCmdBar = CommandBar_Create(hInst, hWnd, 1);
  CommandBar_InsertMenubarEx(hCmdBar, hInst, wcMenu, 0);
  CommandBar_Show(hCmdBar, TRUE);
  topPadding = CommandBar_Height(hCmdBar);
  hMenu = CommandBar_GetMenu(hCmdBar, 0);
#else
  hMenu = LoadMenu(hInst, L"Menu");
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
  for (i = 0; i < sizeof(hCmdBtn) / sizeof(hCmdBtn[0]); ++i) {
    hCmdBtn[i] = CreateWindowExW(0, L"BUTTON", wcCmdBtn[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                 0, 0, 0, 0, hWnd, (HMENU)(IDC_CMDBTN_FIRST + i), hInst, NULL);
  }

  // Screen keyboard
  for (i = 0; i < sizeof(hScrKB) / sizeof(hScrKB[0]); ++i) {
    hScrKB[i] = CreateWindowExW(0, L"BUTTON", wcScrKB[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0,
                                0, 0, 0, hWnd, (HMENU)(IDC_SCRKBD_FIRST + i), hInst, NULL);
  }

  memset(memCache, -1, sizeof(memCache));
}

void onDestroy() {
  DeleteObject(hBtnFont);
  DeleteObject(hEditFont);
  if (retEditBuf) delete[] retEditBuf;
  if (retInBuf) delete[] retInBuf;
}

void onSize() {
  size_t i;
  LOGFONTW rLogfont;
  RECT rect;
  GetClientRect(hWnd, &rect);
  scrX = rect.right;
  scrY = rect.bottom - topPadding;

  // Button font
  rLogfont.lfHeight = mymin(adjustX(16), adjustY(16));
  rLogfont.lfWidth = 0;
  rLogfont.lfEscapement = 0;
  rLogfont.lfOrientation = 0;
  rLogfont.lfWeight = FW_NORMAL;
  rLogfont.lfItalic = FALSE;
  rLogfont.lfUnderline = FALSE;
  rLogfont.lfStrikeOut = FALSE;
  rLogfont.lfCharSet = DEFAULT_CHARSET;
  rLogfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
  rLogfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  rLogfont.lfQuality = DEFAULT_QUALITY;
  rLogfont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
  lstrcpyW(rLogfont.lfFaceName, L"MS Shell Dlg");
  HFONT newBtnFont = CreateFontIndirectW(&rLogfont);

  // Editor/Output font
  rLogfont.lfHeight = mymin(adjustX(16), adjustY(16));
  if (rLogfont.lfHeight < 12) rLogfont.lfHeight = 12;  // Lower bound
  rLogfont.lfWidth = 0;
  rLogfont.lfEscapement = 0;
  rLogfont.lfOrientation = 0;
  rLogfont.lfWeight = FW_NORMAL;
  rLogfont.lfItalic = FALSE;
  rLogfont.lfUnderline = FALSE;
  rLogfont.lfStrikeOut = FALSE;
  rLogfont.lfCharSet = DEFAULT_CHARSET;
  rLogfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
  rLogfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  rLogfont.lfQuality = DEFAULT_QUALITY;
  rLogfont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
  lstrcpyW(rLogfont.lfFaceName, L"MS Shell Dlg");
  HFONT newEditFont = CreateFontIndirectW(&rLogfont);

  // Move and resize the controls
#ifdef UNDER_CE
  MoveWindow(hCmdBar, 0, 0, 0, 0, TRUE);
#endif
  int curX = 0;
  for (i = 0; i < sizeof(hCmdBtn) / sizeof(hCmdBtn[0]); ++i) {
    MoveWindow(hCmdBtn[i], curX, topPadding, adjustX(48), adjustY(32), TRUE);
    SendMessageW(hCmdBtn[i], WM_SETFONT, (WPARAM)newBtnFont, MAKELPARAM(FALSE, 0));
    curX += adjustX(48);
  }
  for (i = 0; i < sizeof(hScrKB) / sizeof(hScrKB[0]); ++i) {
    MoveWindow(hScrKB[i], curX, topPadding, adjustX(32), adjustY(32), TRUE);
    SendMessageW(hScrKB[i], WM_SETFONT, (WPARAM)newBtnFont, MAKELPARAM(FALSE, 0));
    curX += adjustX(32);
  }

  MoveWindow(hEditor, 0, topPadding + adjustY(32), scrX,
             scrY - adjustY(32) - adjustY(64) - adjustY(64), TRUE);
  SendMessageW(hEditor, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(FALSE, 0));
  MoveWindow(hInput, 0, scrY + topPadding - adjustY(64) - adjustY(64), scrX / 2, adjustY(64), TRUE);
  SendMessageW(hInput, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(FALSE, 0));
  MoveWindow(hOutput, scrX / 2, scrY + topPadding - adjustY(64) - adjustY(64), scrX - scrX / 2,
             adjustY(64), TRUE);
  SendMessageW(hOutput, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(FALSE, 0));
  MoveWindow(hMemView, 0, scrY + topPadding - adjustY(64), scrX, adjustY(64), TRUE);
  SendMessageW(hMemView, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(FALSE, 0));

  if (hBtnFont) DeleteObject(hBtnFont);
  if (hEditFont) DeleteObject(hEditFont);
  hBtnFont = newBtnFont;
  hEditFont = newEditFont;
  InvalidateRect(hWnd, NULL, TRUE);
}

void onActivate() { SetFocus(hEditor); }

void onInitMenuPopup() {
  // Options -> Memory type
  CheckMenuRadioItem(hMenu, IDM_OPT_MEMTYPE_SIGNED, IDM_OPT_MEMTYPE_UNSIGNED,
                     signedness ? IDM_OPT_MEMTYPE_SIGNED : IDM_OPT_MEMTYPE_UNSIGNED, MF_BYCOMMAND);

  // Options -> Output charset
  CheckMenuRadioItem(hMenu, IDM_OPT_OUTPUT_ASCII, IDM_OPT_OUTPUT_HEX, outCharSet, MF_BYCOMMAND);

  // Options -> Input charset
  CheckMenuRadioItem(hMenu, IDM_OPT_INPUT_UTF8, IDM_OPT_INPUT_HEX, inCharSet, MF_BYCOMMAND);

  // Options -> Input instruction
  CheckMenuRadioItem(hMenu, IDM_OPT_NOINPUT_ERROR, IDM_OPT_NOINPUT_FF,
                     IDM_OPT_NOINPUT_ERROR + noInput, MF_BYCOMMAND);

  // Options -> Integer overflow
  CheckMenuRadioItem(hMenu, IDM_OPT_INTOVF_ERROR, IDM_OPT_INTOVF_WRAPAROUND,
                     wrapInt ? IDM_OPT_INTOVF_WRAPAROUND : IDM_OPT_INTOVF_ERROR, MF_BYCOMMAND);

  // Options -> Debug
  CheckMenuItem(hMenu, IDM_OPT_DEBUG, MF_BYCOMMAND | debug ? MF_CHECKED : MF_UNCHECKED);

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

  // Options -> Word wrap
  CheckMenuItem(hMenu, IDM_OPT_WORDWRAP, MF_BYCOMMAND | wordwrap ? MF_CHECKED : MF_UNCHECKED);

  if (state == STATE_INIT) {
    EnableMenuItem(hMenu, IDM_FILE_NEW, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(hMenu, IDM_FILE_OPEN, MF_BYCOMMAND | MF_ENABLED);
    enableAllSubMenu(IDM_OPT_MEMTYPE_UNSIGNED, true);
    enableAllSubMenu(IDM_OPT_OUTPUT_HEX, true);
    enableAllSubMenu(IDM_OPT_INPUT_HEX, true);
    enableAllSubMenu(IDM_OPT_NOINPUT_FF, true);
    enableAllSubMenu(IDM_OPT_INTOVF_WRAPAROUND, true);
    enableAllSubMenu(IDM_OPT_DEBUG, true);
    enableAllSubMenu(IDM_OPT_SPEED_100MS, true);
    enableAllSubMenu(IDM_OPT_MEMVIEW, true);
    enableAllSubMenu(IDM_OPT_HIGHLIGHT_MEMORY, false);
    enableAllSubMenu(IDM_OPT_WORDWRAP, true);
  } else if (state == STATE_RUN) {
    EnableMenuItem(hMenu, IDM_FILE_NEW, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMenu, IDM_FILE_OPEN, MF_BYCOMMAND | MF_GRAYED);
    enableAllSubMenu(IDM_OPT_MEMTYPE_UNSIGNED, false);
    enableAllSubMenu(IDM_OPT_OUTPUT_HEX, false);
    enableAllSubMenu(IDM_OPT_INPUT_HEX, false);
    enableAllSubMenu(IDM_OPT_NOINPUT_FF, false);
    enableAllSubMenu(IDM_OPT_INTOVF_WRAPAROUND, false);
    enableAllSubMenu(IDM_OPT_DEBUG, true);
    enableAllSubMenu(IDM_OPT_SPEED_100MS, false);
    enableAllSubMenu(IDM_OPT_MEMVIEW, true);
    enableAllSubMenu(IDM_OPT_HIGHLIGHT_MEMORY, false);
    enableAllSubMenu(IDM_OPT_WORDWRAP, true);
  } else if (state == STATE_PAUSE || state == STATE_FINISH) {
    EnableMenuItem(hMenu, IDM_FILE_NEW, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMenu, IDM_FILE_OPEN, MF_BYCOMMAND | MF_GRAYED);
    enableAllSubMenu(IDM_OPT_MEMTYPE_UNSIGNED, false);
    enableAllSubMenu(IDM_OPT_OUTPUT_HEX, false);
    enableAllSubMenu(IDM_OPT_INPUT_HEX, false);
    enableAllSubMenu(IDM_OPT_NOINPUT_FF, false);
    enableAllSubMenu(IDM_OPT_INTOVF_WRAPAROUND, false);
    enableAllSubMenu(IDM_OPT_DEBUG, true);
    enableAllSubMenu(IDM_OPT_SPEED_100MS, true);
    enableAllSubMenu(IDM_OPT_MEMVIEW, true);
    enableAllSubMenu(IDM_OPT_HIGHLIGHT_MEMORY, true);
    enableAllSubMenu(IDM_OPT_WORDWRAP, true);
  }
}

#ifndef UNDER_CE
// WM_DROPFILES handler.
void onDropFiles(HDROP hDrop) {
  wchar_t wcFileName[MAX_PATH];
  DragQueryFileW(hDrop, 0, wcFileName, MAX_PATH);
  DragFinish(hDrop);
  openFile(false, wcFileName);
}
#endif

void onScreenKeyboard(int _key) { SendMessageW(hEditor, EM_REPLACESEL, 0, (WPARAM)wcScrKB[_key]); }

static LRESULT CALLBACK memViewDlgEditor(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  static WNDPROC prevWndProc = (WNDPROC)myGetWindowLongW(hWnd, GWL_USERDATA);

  switch (uMsg) {
    case WM_CHAR:
      switch ((CHAR)wParam) {
        case 'q':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"1");
          return 0;

        case 'w':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"2");
          return 0;

        case 'e':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"3");
          return 0;

        case 'r':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"4");
          return 0;

        case 't':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"5");
          return 0;

        case 'y':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"6");
          return 0;

        case 'u':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"7");
          return 0;

        case 'i':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"8");
          return 0;

        case 'o':
          SendMessageW(hWnd, EM_REPLACESEL, 0, (WPARAM)L"9");
          return 0;

        case 'p':
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

INT_PTR CALLBACK memViewProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
  UNREFERENCED_PARAMETER(lParam);

  switch (uMsg) {
    case WM_INITDIALOG: {
      wchar_t editBuf[10];
      HWND hEdit = GetDlgItem(hDlg, IDG_FROM);
      wsprintfW(editBuf, L"%u", memViewStart <= 999999999 ? memViewStart : 999999999);
      SendDlgItemMessageW(hDlg, IDG_FROM, EM_SETLIMITTEXT, 9, 0);
      SetDlgItemTextW(hDlg, IDG_FROM, editBuf);
      mySetWindowLongW(hEdit, GWL_USERDATA, mySetWindowLongW(hEdit, GWL_WNDPROC, memViewDlgEditor));
      return TRUE;
    }

    case WM_CLOSE:
      EndDialog(hDlg, IDCANCEL);
      return TRUE;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDG_OK: {
          wchar_t editBuf[10];
          long temp;
          GetDlgItemTextW(hDlg, IDG_FROM, (wchar_t *)editBuf, 10);
          temp = wcstol(editBuf, NULL, 10);
          if (temp < 0)
            MessageBoxW(hDlg, L"Invalid input.", L"Error", MB_ICONWARNING);
          else {
            memViewStart = temp;
            EndDialog(hDlg, IDOK);
          }
          return TRUE;
        }

        case IDG_CANCEL:
          EndDialog(hDlg, IDCANCEL);
          return TRUE;
      }
      return FALSE;
  }
  return FALSE;
}

void setState(enum state_t _state, bool _force) {
  size_t i;
  if (!_force && _state == state) return;

  if (_state == STATE_INIT) {
    EnableWindow(hCmdBtn[0], TRUE);   // run button
    EnableWindow(hCmdBtn[1], TRUE);   // next button
    EnableWindow(hCmdBtn[2], FALSE);  // pause button
    EnableWindow(hCmdBtn[3], FALSE);  // end button
    SendMessageW(hEditor, EM_SETREADONLY, (WPARAM)FALSE, (LPARAM)NULL);
    SendMessageW(hInput, EM_SETREADONLY, (WPARAM)FALSE, (LPARAM)NULL);
    SendMessageW(hOutput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    for (i = 0; i < sizeof(hScrKB) / sizeof(hScrKB[0]); ++i) {
      EnableWindow(hScrKB[i], TRUE);
    }
    memset(memCache, -1, sizeof(memCache));
  } else if (_state == STATE_RUN) {
    EnableWindow(hCmdBtn[0], FALSE);  // run button
    EnableWindow(hCmdBtn[1], FALSE);  // next button
    EnableWindow(hCmdBtn[2], TRUE);   // pause button
    EnableWindow(hCmdBtn[3], TRUE);   // end button
    SendMessageW(hEditor, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(hInput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(hOutput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    for (i = 0; i < sizeof(hScrKB) / sizeof(hScrKB[0]); ++i) {
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
    for (i = 0; i < sizeof(hScrKB) / sizeof(hScrKB[0]); ++i) {
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
    for (i = 0; i < sizeof(hScrKB) / sizeof(hScrKB[0]); ++i) {
      EnableWindow(hScrKB[i], FALSE);
    }
  }

  state = _state;
  InvalidateRect(hWnd, NULL, FALSE);
}

void setFocus() { SetFocus(hEditor); }

void setSelect(unsigned int _progPtr) { SendMessageW(hEditor, EM_SETSEL, _progPtr, _progPtr + 1); }

void setSelectMemView(unsigned int _memPtr) {
  SendMessageW(hMemView, EM_SETSEL, (_memPtr - memViewStart) * 3, (_memPtr - memViewStart) * 3 + 2);
}

void setMemory(const std::vector<unsigned char> *memory) {
  static unsigned int prevStart = 0;
  bool isCacheValid = prevStart == memViewStart;

  if (!memory) {
    SetWindowTextW(hMemView, NULL);
    memset(memCache, -1, sizeof(memCache));
    return;
  }

  unsigned int i;
  for (i = memViewStart; i < memViewStart + 100 && i < memory->size(); ++i) {
    if (isCacheValid && memCache[i - memViewStart] == memory->at(i)) continue;
    memCache[i - memViewStart] = memory->at(i);
    wchar_t wcOut[] = {0, 0, L' ', 0};
    unsigned char high = memory->at(i) >> 4, low = memory->at(i) & 0xF;
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
    SendMessageW(hMemView, EM_SETSEL, (i - memViewStart) * 3, (i - memViewStart + 1) * 3);
    SendMessageW(hMemView, EM_REPLACESEL, 0, (WPARAM)wcOut);
  }
  isCacheValid = true;
  SendMessageW(hMemView, EM_SETSEL, -1, 0);
}

wchar_t *getEditor() {
  int editorSize = GetWindowTextLengthW(hEditor) + 1;
  if (retEditBuf) delete[] retEditBuf;
  retEditBuf = new wchar_t[editorSize];
  GetWindowTextW(hEditor, retEditBuf, editorSize);

  return retEditBuf;
}

wchar_t *getInput() {
  int inputSize = GetWindowTextLengthW(hInput) + 1;
  if (retInBuf) delete[] retInBuf;
  retInBuf = new wchar_t[inputSize];
  GetWindowTextW(hInput, retInBuf, inputSize);

  return retInBuf;
}

void clearOutput() { SetWindowTextW(hOutput, L""); }

void setOutput(const wchar_t *_str) { SetWindowTextW(hOutput, _str); }

void appendOutput(const wchar_t *_str) {
  int editLen = (int)SendMessageW(hOutput, WM_GETTEXTLENGTH, 0, 0);
  SendMessageW(hOutput, EM_SETSEL, editLen, editLen);
  SendMessageW(hOutput, EM_REPLACESEL, 0, (WPARAM)_str);
}

void switchWordwrap() {
  wordwrap = !wordwrap;

  int editorSize = GetWindowTextLengthW(hEditor) + 1;
  wchar_t *wcEditor = new wchar_t[editorSize];
  GetWindowTextW(hEditor, wcEditor, editorSize);
  bool isModified = SendMessageW(hEditor, EM_GETMODIFY, 0, 0) != 0;
  DestroyWindow(hEditor);

  int inputSize = GetWindowTextLengthW(hInput) + 1;
  wchar_t *wcInput = new wchar_t[inputSize];
  GetWindowTextW(hInput, wcInput, inputSize);
  DestroyWindow(hInput);

  int outputSize = GetWindowTextLengthW(hOutput) + 1;
  wchar_t *wcOutput = new wchar_t[outputSize];
  GetWindowTextW(hOutput, wcOutput, outputSize);
  DestroyWindow(hOutput);

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

  onSize();
  setState(state, true);

  SetWindowTextW(hEditor, wcEditor);
  SendMessageW(hEditor, EM_SETMODIFY, isModified, 0);
  SetWindowTextW(hInput, wcInput);
  SetWindowTextW(hOutput, wcOutput);

  delete[] wcEditor;
  delete[] wcInput;
  delete[] wcOutput;
}

bool promptSave() {
  if (SendMessageW(hEditor, EM_GETMODIFY, 0, 0) == 0) return true;

  int ret = MessageBoxW(hWnd, L"Unsaved data will be lost. Save changes?", L"Confirm",
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

  // shouldn't be reached
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
    MessageBoxW(hWnd, L"Open failed.", L"Error", MB_ICONWARNING);
    return;
  }

  DWORD fileSize = GetFileSize(hFile, NULL), readLen;
  char *fileBuf = new char[fileSize];
  int padding = 0;
  ReadFile(hFile, fileBuf, fileSize, &readLen, NULL);
  if (fileBuf[0] == '\xEF' && fileBuf[1] == '\xBB' && fileBuf[2] == '\xBF') padding = 3;
  int length = MultiByteToWideChar(CP_UTF8, 0, fileBuf + padding, readLen - padding, NULL, 0);
  if (length >= 0x7FFFFFFE) {
    MessageBoxW(hWnd, L"This file is too large.", L"Error", MB_ICONWARNING);
    delete[] fileBuf;
    CloseHandle(hFile);
    return;
  }
  wchar_t *wcFileBuf = new wchar_t[length + 1]();
  MultiByteToWideChar(CP_UTF8, 0, fileBuf + padding, readLen - padding, wcFileBuf, length);

  SetWindowTextW(hEditor, wcFileBuf);
  SendMessageW(hEditor, EM_SETMODIFY, FALSE, 0);
  wstrFileName = _fileName;
  withBOM = padding != 0;

  CloseHandle(hFile);
  delete[] fileBuf;
  delete[] wcFileBuf;

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

  HANDLE hFile = CreateFileW(wcFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    MessageBoxW(hWnd, L"Open failed.", L"Error", MB_ICONWARNING);
    return false;
  }

  int editorSize = GetWindowTextLengthW(hEditor) + 1;
  wchar_t *wcEditor = new wchar_t[editorSize];
  GetWindowTextW(hEditor, wcEditor, editorSize);
  int length = WideCharToMultiByte(CP_UTF8, 0, wcEditor, -1, NULL, 0, NULL, NULL);
  char *szEditor = new char[length];
  WideCharToMultiByte(CP_UTF8, 0, wcEditor, -1, szEditor, length, NULL, NULL);
  DWORD dwTemp;
  if (withBOM) WriteFile(hFile, "\xEF\xBB\xBF", 3, &dwTemp, NULL);
  WriteFile(hFile, szEditor, length - 1, &dwTemp, NULL);

  SendMessageW(hEditor, EM_SETMODIFY, FALSE, 0);
  wstrFileName = wcFileName;

  CloseHandle(hFile);
  delete[] wcEditor;
  delete[] szEditor;

  std::wstring title = L"[";
  title.append(wstrFileName.substr(wstrFileName.rfind(L'\\') + 1) + L"] - " APP_NAME);
  SetWindowTextW(hWnd, title.c_str());

  return true;
}
}  // namespace ui
