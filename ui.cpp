#include "ui.hpp"

#include "msg.hpp"
#include "wproc.hpp"

namespace ui {
void enableMenus(unsigned endID, bool enable) {
  unsigned i;
  for (i = (endID / 10) * 10; i <= endID; ++i) {
    EnableMenuItem(global::hMenu, i, MF_BYCOMMAND | (enable ? MF_ENABLED : MF_GRAYED));
  }
}

void setState(enum global::state_t state, bool force) {
  int i;
  if (!force && state == global::state) return;

  if (state == global::STATE_INIT) {
    EnableWindow(global::hCmdBtn[0], TRUE);   // run button
    EnableWindow(global::hCmdBtn[1], TRUE);   // next button
    EnableWindow(global::hCmdBtn[2], FALSE);  // pause button
    EnableWindow(global::hCmdBtn[3], FALSE);  // end button
    SendMessageW(global::hEditor, EM_SETREADONLY, (WPARAM)FALSE, (LPARAM)NULL);
    SendMessageW(global::hInput, EM_SETREADONLY, (WPARAM)FALSE, (LPARAM)NULL);
    SendMessageW(global::hOutput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    for (i = 0; i < SCRKBD_LEN; ++i) EnableWindow(global::hScrKB[i], TRUE);
#ifndef UNDER_CE
    DragAcceptFiles(global::hWnd, TRUE);
#endif
  } else if (state == global::STATE_RUN) {
    EnableWindow(global::hCmdBtn[0], FALSE);  // run button
    EnableWindow(global::hCmdBtn[1], FALSE);  // next button
    EnableWindow(global::hCmdBtn[2], TRUE);   // pause button
    EnableWindow(global::hCmdBtn[3], TRUE);   // end button
    SendMessageW(global::hEditor, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(global::hInput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(global::hOutput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    for (i = 0; i < SCRKBD_LEN; ++i) EnableWindow(global::hScrKB[i], FALSE);
#ifndef UNDER_CE
    DragAcceptFiles(global::hWnd, FALSE);
#endif
  } else if (state == global::STATE_PAUSE) {
    EnableWindow(global::hCmdBtn[0], TRUE);   // run button
    EnableWindow(global::hCmdBtn[1], TRUE);   // next button
    EnableWindow(global::hCmdBtn[2], FALSE);  // pause button
    EnableWindow(global::hCmdBtn[3], TRUE);   // end button
    SendMessageW(global::hEditor, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(global::hInput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(global::hOutput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    for (i = 0; i < SCRKBD_LEN; ++i) EnableWindow(global::hScrKB[i], FALSE);
#ifndef UNDER_CE
    DragAcceptFiles(global::hWnd, FALSE);
#endif
  } else if (state == global::STATE_FINISH) {
    EnableWindow(global::hCmdBtn[0], FALSE);  // run button
    EnableWindow(global::hCmdBtn[1], FALSE);  // next button
    EnableWindow(global::hCmdBtn[2], FALSE);  // pause button
    EnableWindow(global::hCmdBtn[3], TRUE);   // end button
    SendMessageW(global::hEditor, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(global::hInput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    SendMessageW(global::hOutput, EM_SETREADONLY, (WPARAM)TRUE, (LPARAM)NULL);
    for (i = 0; i < SCRKBD_LEN; ++i) EnableWindow(global::hScrKB[i], FALSE);
#ifndef UNDER_CE
    DragAcceptFiles(global::hWnd, FALSE);
#endif
  }

  global::state = state;
  InvalidateRect(global::hWnd, NULL, FALSE);
}

void updateFocus(HWND hWndCtl) {
  if (hWndCtl == global::hEditor || hWndCtl == global::hInput || hWndCtl == global::hOutput ||
      hWndCtl == global::hMemView) {
    global::hFocused = hWndCtl;
  } else {
    SetFocus(global::hFocused);
  }
}

void selProg(unsigned progPtr) { SendMessageW(global::hEditor, EM_SETSEL, progPtr, progPtr + 1); }

void selMemView(unsigned memPtr) {
  SendMessageW(global::hMemView, EM_SETSEL, (memPtr - global::memViewStart) * 3,
               (memPtr - global::memViewStart) * 3 + 2);
}

void setMemory(const unsigned char *memory, int size) {
  int i;
  std::wstring wstrOut;
  for (i = global::memViewStart; i < global::memViewStart + 100 && i < size; ++i) {
    wchar_t wcOut[4];
    util::toHex(memory[i], wcOut);
    wstrOut.append(wcOut);
  }
  SetWindowTextW(global::hMemView, wstrOut.c_str());
}

void appendOutput(const wchar_t *str) {
  int editLen = (int)SendMessageW(global::hOutput, WM_GETTEXTLENGTH, 0, 0);
  SendMessageW(global::hOutput, EM_SETSEL, editLen, editLen);
  SendMessageW(global::hOutput, EM_REPLACESEL, 0, (WPARAM)str);
}

void switchWordwrap() {
  int editorSize = GetWindowTextLengthW(global::hEditor) + 1;
  wchar_t *wcEditor = (wchar_t *)malloc(sizeof(wchar_t) * editorSize);
  if (!wcEditor) {
    util::messageBox(global::hWnd, global::hInst, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return;
  }
  GetWindowTextW(global::hEditor, wcEditor, editorSize);
  bool isModified = SendMessageW(global::hEditor, EM_GETMODIFY, 0, 0) != 0;
  DestroyWindow(global::hEditor);

  int inputSize = GetWindowTextLengthW(global::hInput) + 1;
  wchar_t *wcInput = (wchar_t *)malloc(sizeof(wchar_t) * inputSize);
  if (!wcInput) {
    free(wcEditor);
    util::messageBox(global::hWnd, global::hInst, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return;
  }
  GetWindowTextW(global::hInput, wcInput, inputSize);
  DestroyWindow(global::hInput);

  int outputSize = GetWindowTextLengthW(global::hOutput) + 1;
  wchar_t *wcOutput = (wchar_t *)malloc(sizeof(wchar_t) * outputSize);
  if (!wcOutput) {
    free(wcEditor);
    free(wcInput);
    util::messageBox(global::hWnd, global::hInst, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return;
  }
  GetWindowTextW(global::hOutput, wcOutput, outputSize);
  DestroyWindow(global::hOutput);

  global::wordwrap = !global::wordwrap;

  // Program editor
  global::hEditor =
      CreateWindowExW(0, L"EDIT", L"",
                      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL |
                          (global::wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL) | ES_NOHIDESEL,
                      0, 0, 0, 0, global::hWnd, (HMENU)IDC_EDITOR, global::hInst, NULL);
  SendMessageW(global::hInput, EM_SETLIMITTEXT, (WPARAM)-1, 0);
  mySetWindowLongW(global::hEditor, GWL_USERDATA, mySetWindowLongW(global::hEditor, GWL_WNDPROC, wproc::editorProc));
  global::history.changeEditor(global::hEditor);

  // Program input
  global::hInput = CreateWindowExW(0, L"EDIT", L"",
                                   WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                                       WS_VSCROLL | (global::wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL) | ES_NOHIDESEL,
                                   0, 0, 0, 0, global::hWnd, (HMENU)IDC_INPUT, global::hInst, NULL);
  SendMessageW(global::hInput, EM_SETLIMITTEXT, (WPARAM)-1, 0);
  mySetWindowLongW(global::hInput, GWL_USERDATA, mySetWindowLongW(global::hInput, GWL_WNDPROC, wproc::inputProc));
  global::inputHistory.changeEditor(global::hInput);

  // Program output
  global::hOutput =
      CreateWindowExW(0, L"EDIT", L"",
                      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL |
                          WS_VSCROLL | (global::wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL) | ES_NOHIDESEL,
                      0, 0, 0, 0, global::hWnd, (HMENU)IDC_OUTPUT, global::hInst, NULL);
  SendMessageW(global::hOutput, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  global::hFocused = global::hEditor;
  ui::updateFocus();
  msg::onSize(global::hWnd);
  ui::setState(global::state, true);

  SetWindowTextW(global::hEditor, wcEditor);
  SendMessageW(global::hEditor, EM_SETMODIFY, isModified, 0);
  SetWindowTextW(global::hInput, wcInput);
  SetWindowTextW(global::hOutput, wcOutput);

  free(wcEditor);
  free(wcInput);
  free(wcOutput);
}

void switchTheme() {
  global::dark = !global::dark;

  InvalidateRect(global::hEditor, NULL, TRUE);
  InvalidateRect(global::hInput, NULL, TRUE);
  InvalidateRect(global::hOutput, NULL, TRUE);
  InvalidateRect(global::hMemView, NULL, TRUE);
}

void switchLayout() {
  global::horizontal = !global::horizontal;

  msg::onSize(global::hWnd);
}

void updateTitle() {
  std::wstring title;

  title += L"[";
  title +=
      global::wstrFileName.empty() ? L"New File" : global::wstrFileName.substr(global::wstrFileName.rfind(L'\\') + 1);
  title += global::history.isSaved() ? L"] - " : L" *] - ";

  title += APP_NAME;

  SetWindowTextW(global::hWnd, title.c_str());
}

void chooseFont() {
#ifndef UNDER_CE
  LONG origHeight = global::editFont.lfHeight;
  // Temporarilly sets to the "System DPI scaled" value since ChooseFontW expects it.
  // Subtracting by 96 - 1 makes this division to behave like a ceiling function.
  // Note that editFont.lfHeight is negative.
  global::editFont.lfHeight = (global::editFont.lfHeight * global::sysDPI - (96 - 1)) / 96;
#endif

  CHOOSEFONTW cf;
  cf.lStructSize = sizeof(CHOOSEFONTW);
  cf.hwndOwner = global::hWnd;
  cf.lpLogFont = &global::editFont;
  cf.hDC = GetDC(global::hWnd);  // Required for CF_BOTH.
  // We won't get any fonts if we omit CF_BOTH on Windows CE.
  cf.Flags = CF_INITTOLOGFONTSTRUCT | CF_FORCEFONTEXIST | CF_BOTH;
  BOOL ret = ChooseFontW(&cf);
  ReleaseDC(global::hWnd, cf.hDC);

#ifdef UNDER_CE
  if (ret) {
    global::editFont.lfQuality = ANTIALIASED_QUALITY;
    msg::onSize(global::hWnd);
  }
#else
  if (ret) {
    // Re-converts to the 96 DPI value as we properly adjust it on the fly.
    global::editFont.lfHeight = (global::editFont.lfHeight * 96 - (global::sysDPI - 1)) / global::sysDPI;
    global::editFont.lfQuality = ANTIALIASED_QUALITY;
    msg::onSize(global::hWnd);
  } else {
    // Rewrites the original value instead of re-converting.
    // Re-converting can results in a different value.
    global::editFont.lfHeight = origHeight;
  }
#endif
}

bool promptSave() {
  if (global::history.isSaved()) return true;

  int ret = util::messageBox(global::hWnd, global::hInst, L"Unsaved data will be lost. Save changes?", L"Confirm",
                             MB_ICONWARNING | MB_YESNOCANCEL);

  return ret == IDCANCEL ? false : ret == IDYES ? saveFile(true) : true;
}

void openFile(bool newFile, const wchar_t *fileName) {
  wchar_t wcFileName[MAX_PATH] = {0};

  if (!promptSave()) return;

  if (newFile) {  // new
    SetWindowTextW(global::hEditor, L"");
    updateTitle();
    global::wstrFileName = L"";
    global::withBOM = false;
    global::newLine = util::NEWLINE_CRLF;
    global::history.reset(global::hEditor);
    return;
  }

  if (!fileName || !fileName[0]) {
    wchar_t initDir[MAX_PATH] = {0};
    if (!global::wstrFileName.empty()) {
      global::wstrFileName.substr(0, global::wstrFileName.rfind(L'\\')).copy(initDir, MAX_PATH - 1);
    }
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = global::hWnd;
    ofn.lpstrFilter = L"Brainfuck source (*.bf;*.b;*.txt)\0*.bf;*.b;*.txt\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = wcFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L".bf";
    ofn.lpstrInitialDir = initDir[0] ? initDir : NULL;
    ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST;
    if (!GetOpenFileNameW(&ofn)) return;
    fileName = wcFileName;
  }

  HANDLE hFile = CreateFileW(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    util::messageBox(global::hWnd, global::hInst, L"Open failed.", L"Error", MB_ICONWARNING);
    return;
  }

  DWORD fileSize = GetFileSize(hFile, NULL), readLen;
  if (fileSize >= 65536 * 2) {
    util::messageBox(global::hWnd, global::hInst, L"This file is too large.", L"Error", MB_ICONWARNING);
    return;
  }

  char *fileBuf = (char *)malloc(sizeof(char) * fileSize + 1);
  if (!fileBuf) {
    util::messageBox(global::hWnd, global::hInst, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
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
    util::messageBox(global::hWnd, global::hInst, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return;
  }

  MultiByteToWideChar(CP_UTF8, 0, fileBuf + padding, readLen - padding, wcFileBuf, length);
  free(fileBuf);

  std::wstring converted = wcFileBuf;
  free(wcFileBuf);
  global::newLine = util::convertCRLF(converted, util::NEWLINE_CRLF);

  SetWindowTextW(global::hEditor, converted.c_str());
  global::wstrFileName = fileName;
  global::withBOM = padding != 0;
  global::history.reset(global::hEditor, converted.c_str());

  SetWindowTextW(global::hOutput, NULL);
  SetWindowTextW(global::hMemView, NULL);
  updateTitle();
}

bool saveFile(bool isOverwrite) {
  wchar_t wcFileName[MAX_PATH] = {0};

  if (!isOverwrite || global::wstrFileName.empty()) {
    wchar_t initDir[MAX_PATH] = {0};
    if (!global::wstrFileName.empty()) {
      global::wstrFileName.substr(global::wstrFileName.rfind(L'\\') + 1).copy(wcFileName, MAX_PATH - 1);
      global::wstrFileName.substr(0, global::wstrFileName.rfind(L'\\')).copy(initDir, MAX_PATH - 1);
    }
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(OPENFILENAMEW));
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = global::hWnd;
    ofn.lpstrFilter = L"Brainfuck source (*.bf;*.b;*.txt)\0*.bf;*.b;*.txt\0All files (*.*)\0*.*\0";
    ofn.lpstrFile = wcFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = L".bf";
    ofn.lpstrInitialDir = initDir[0] ? initDir : NULL;
    ofn.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    if (!GetSaveFileNameW(&ofn)) return false;
  } else {
    global::wstrFileName.copy(wcFileName, MAX_PATH);
  }

  int editorSize = GetWindowTextLengthW(global::hEditor) + 1;
  wchar_t *wcEditor = (wchar_t *)malloc(sizeof(wchar_t) * editorSize);
  if (!wcEditor) {
    util::messageBox(global::hWnd, global::hInst, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return false;
  }
  GetWindowTextW(global::hEditor, wcEditor, editorSize);
  std::wstring converted = wcEditor;
  free(wcEditor);

  if (global::newLine != util::NEWLINE_CRLF) util::convertCRLF(converted, global::newLine);
  int length = WideCharToMultiByte(CP_UTF8, 0, converted.c_str(), -1, NULL, 0, NULL, NULL);
  char *szEditor = (char *)malloc(sizeof(char) * length);
  if (!szEditor) {
    util::messageBox(global::hWnd, global::hInst, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    return false;
  }
  WideCharToMultiByte(CP_UTF8, 0, converted.c_str(), -1, szEditor, length, NULL, NULL);

  HANDLE hFile =
      CreateFileW(wcFileName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    util::messageBox(global::hWnd, global::hInst, L"Open failed.", L"Error", MB_ICONWARNING);
    return false;
  }

  DWORD dwTemp;
  if (global::withBOM) WriteFile(hFile, "\xEF\xBB\xBF", 3, &dwTemp, NULL);  // BOM
  WriteFile(hFile, szEditor, length - 1, &dwTemp, NULL);
  CloseHandle(hFile);
  free(szEditor);

  global::history.setSaved();
  global::wstrFileName = wcFileName;
  updateTitle();

  return true;
}
}  // namespace ui
