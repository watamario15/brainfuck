#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef UNICODE
#define UNICODE
#endif
#define NOMINMAX
#include <windows.h>
#define mymin(a, b) (((a) < (b)) ? (a) : (b))

#ifdef UNDER_CE
#include <commctrl.h>
#include <commdlg.h>
static HWND hCmdBar;
#endif

#include <string>

#include "bf.hpp"
#include "def.h"
#include "ui.hpp"

namespace ui {
bool signedness = true, wrapInt = true, wrapPtr = false;
int speed = 10, outCharSet = IDM_OPT_OUTPUT_ASCII, inCharSet = IDM_OPT_INPUT_UTF8;
enum bf::noinput_t noInput = bf::NOINPUT_ZERO;
HWND hWnd;
static HINSTANCE hInst;
static HWND hEditor, hInput, hOutput, hCmdBtn[4], hScrKB[8];
static HMENU hMenu;
static HFONT hBtnFont = NULL, hEditFont = NULL;
static int topPadding = 0, scrX = 480, scrY = 320;
static enum state_t state = STATE_INIT;
static wchar_t *retEditBuf = NULL, *retInBuf = NULL;
static const wchar_t *wcCmdBtn[] = {L"Run", L"Next", L"Pause", L"End"},
                     *wcScrKB[] = {L">", L"<", L"+", L"-", L".", L",", L"[", L"]"};
static std::wstring wstrFileName;
static bool withBOM = false, wordwrap = false;

static inline int adjustX(int x) { return x * scrX / 480; }
static inline int adjustY(int y) { return y * scrY / 320; }

void onCreate(HWND _hWnd, HINSTANCE _hInst) {
  size_t i;
  hWnd = _hWnd;
  hInst = _hInst;

#ifdef UNDER_CE
  wchar_t wcMenu[] = L"Menu";  // requires to be non-const for whatever reason
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
  hEditor = CreateWindowExW(0, L"EDIT", L"",
                            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE |
                                ES_AUTOVSCROLL | WS_VSCROLL | (wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL),
                            0, 0, 0, 0, hWnd, (HMENU)IDC_EDITOR, hInst, NULL);
  SendMessageW(hEditor, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  // Program input
  hInput = CreateWindowExW(0, L"EDIT", L"",
                           WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE |
                               ES_AUTOVSCROLL | WS_VSCROLL | (wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL),
                           0, 0, 0, 0, hWnd, (HMENU)IDC_INPUT, hInst, NULL);
  SendMessageW(hInput, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  // Program output
  hOutput =
      CreateWindowExW(0, L"EDIT", L"",
                      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_READONLY |
                          ES_AUTOVSCROLL | WS_VSCROLL | (wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL),
                      0, 0, 0, 0, hWnd, (HMENU)IDC_OUTPUT, hInst, NULL);
  SendMessageW(hOutput, EM_SETLIMITTEXT, (WPARAM)-1, 0);

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
  MoveWindow(hInput, 0, scrY + topPadding - adjustY(64) - adjustY(64), scrX, adjustY(64), TRUE);
  SendMessageW(hInput, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(FALSE, 0));
  MoveWindow(hOutput, 0, scrY + topPadding - adjustY(64), scrX, adjustY(64), TRUE);
  SendMessageW(hOutput, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(FALSE, 0));

  if (hBtnFont) DeleteObject(hBtnFont);
  if (hEditFont) DeleteObject(hEditFont);
  hBtnFont = newBtnFont;
  hEditFont = newEditFont;
  InvalidateRect(hWnd, NULL, TRUE);
}

void onPaint() {
  RECT rect;
  PAINTSTRUCT ps;
  GetClientRect(hWnd, &rect);
  HDC hDC = BeginPaint(hWnd, &ps);

  if (state == STATE_INIT) {
    FillRect(hDC, &rect, GetSysColorBrush(COLOR_BTNFACE));
  } else if (state == STATE_RUN) {
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 0));
    FillRect(hDC, &rect, hBrush);
    DeleteObject(hBrush);
  } else if (state == STATE_PAUSE) {
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 255, 255));
    FillRect(hDC, &rect, hBrush);
    DeleteObject(hBrush);
  } else if (state == STATE_FINISH) {
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 255, 0));
    FillRect(hDC, &rect, hBrush);
    DeleteObject(hBrush);
  }

  EndPaint(hWnd, &ps);
}

void onActivate() { SetFocus(hEditor); }

void onInitMenuPopup() {
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

  // Options -> Pointer overflow
  CheckMenuRadioItem(hMenu, IDM_OPT_PTROVF_ERROR, IDM_OPT_PTROVF_WRAPAROUND,
                     wrapPtr ? IDM_OPT_PTROVF_WRAPAROUND : IDM_OPT_PTROVF_ERROR, MF_BYCOMMAND);

  // Options -> Word wrap
  CheckMenuItem(hMenu, IDM_OPT_WORDWRAP, wordwrap);

  HMENU hOptMenu = GetSubMenu(hMenu, 1);
  if (state == STATE_INIT) {
    EnableMenuItem(hMenu, IDM_FILE_NEW, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(hMenu, IDM_FILE_OPEN, MF_BYCOMMAND | MF_ENABLED);
    EnableMenuItem(hOptMenu, 0, MF_BYPOSITION | MF_ENABLED);
    EnableMenuItem(hOptMenu, 1, MF_BYPOSITION | MF_ENABLED);
    EnableMenuItem(hOptMenu, 2, MF_BYPOSITION | MF_ENABLED);
    EnableMenuItem(hOptMenu, 3, MF_BYPOSITION | MF_ENABLED);
    EnableMenuItem(hOptMenu, 4, MF_BYPOSITION | MF_ENABLED);
    EnableMenuItem(hOptMenu, 5, MF_BYPOSITION | MF_ENABLED);
    EnableMenuItem(hOptMenu, 6, MF_BYPOSITION | MF_ENABLED);
    EnableMenuItem(hOptMenu, 7, MF_BYPOSITION | MF_ENABLED);
  } else if (state == STATE_RUN) {
    EnableMenuItem(hMenu, IDM_FILE_NEW, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMenu, IDM_FILE_OPEN, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hOptMenu, 0, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hOptMenu, 1, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hOptMenu, 2, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hOptMenu, 3, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hOptMenu, 4, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hOptMenu, 5, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hOptMenu, 6, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hOptMenu, 7, MF_BYPOSITION | MF_ENABLED);
  } else if (state == STATE_PAUSE || state == STATE_FINISH) {
    EnableMenuItem(hMenu, IDM_FILE_NEW, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hMenu, IDM_FILE_OPEN, MF_BYCOMMAND | MF_GRAYED);
    EnableMenuItem(hOptMenu, 0, MF_BYPOSITION | MF_ENABLED);
    EnableMenuItem(hOptMenu, 1, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hOptMenu, 2, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hOptMenu, 3, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hOptMenu, 4, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hOptMenu, 5, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hOptMenu, 6, MF_BYPOSITION | MF_GRAYED);
    EnableMenuItem(hOptMenu, 7, MF_BYPOSITION | MF_ENABLED);
  }
}

void onScreenKeyboard(int _key) { SendMessageW(hEditor, EM_REPLACESEL, 0, (WPARAM)wcScrKB[_key]); }

void setState(enum state_t _state) {
  size_t i;
  if (_state == state) return;

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
                          WS_VSCROLL | (wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL),
                      0, 0, 0, 0, hWnd, (HMENU)IDC_EDITOR, hInst, NULL);
  SendMessageW(hInput, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  // Program input
  hInput =
      CreateWindowExW(0, L"EDIT", L"",
                      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                          WS_VSCROLL | (wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL),
                      0, 0, 0, 0, hWnd, (HMENU)IDC_INPUT, hInst, NULL);
  SendMessageW(hInput, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  // Program output
  hOutput = CreateWindowExW(0, L"EDIT", L"",
                            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE |
                                ES_READONLY | ES_AUTOVSCROLL | WS_VSCROLL |
                                (wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL),
                            0, 0, 0, 0, hWnd, (HMENU)IDC_OUTPUT, hInst, NULL);
  SendMessageW(hOutput, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  onSize();

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

  std::wstring title = APP_NAME L" - ";
  title.append(wstrFileName.substr(wstrFileName.rfind(L'\\') + 1));
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

  std::wstring title = APP_NAME L" - ";
  title.append(wstrFileName.substr(wstrFileName.rfind(L'\\') + 1));
  SetWindowTextW(hWnd, title.c_str());

  return true;
}
}  // namespace ui
