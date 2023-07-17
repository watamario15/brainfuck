#include "history.hpp"

void History::reset(HWND hEditor, const wchar_t *str) {
  SendMessageW(hEditor, EM_SETMODIFY, FALSE, 0);

  while (!history.empty()) {
    free(history.back());
    history.pop_back();
  }

  wchar_t *newStr;
  if (str) {
    newStr = (wchar_t *)malloc((wcslen(str) + 1) * sizeof(wchar_t));
    if (!newStr) {
      validHistory = false;
      historyIndex = 0;
      savedIndex = -1;
      return;
    }
    wcscpy(newStr, str);
  } else {
    newStr = (wchar_t *)malloc(sizeof(wchar_t));
    if (!newStr) {
      validHistory = false;
      historyIndex = 0;
      savedIndex = -1;
      return;
    }
    newStr[0] = L'\0';
  }

  this->hEditor = hEditor;
  history.push_back(newStr);
  historyIndex = savedIndex = 0;
  validHistory = true;
}

void History::add() {
  if (!validHistory) return;

  int editorSize = GetWindowTextLengthW(hEditor) + 1;
  wchar_t *wcEditor = (wchar_t *)malloc(sizeof(wchar_t) * editorSize);
  if (!wcEditor) {
    while (!history.empty()) {
      free(history.back());
      history.pop_back();
    }
    historyIndex = 0;
    savedIndex = -1;
    validHistory = false;
    return;
  }
  GetWindowTextW(hEditor, wcEditor, editorSize);

  if (!history.empty() && !wcscmp(history[historyIndex], wcEditor)) {
    free(wcEditor);
    return;
  }

  while (historyIndex < (int)history.size() - 1) {
    free(history.back());
    history.pop_back();
  }

  if (history.size() >= MAX_HISTORY) {
    free(history.front());
    history.pop_front();
    --historyIndex;
    if (savedIndex >= 0) --savedIndex;
  }

  history.push_back(wcEditor);
  ++historyIndex;
}

void History::undo() {
  if (canUndo()) {
    DWORD selEnd;
    --historyIndex;
    SendMessageW(hEditor, EM_GETSEL, (WPARAM)NULL, (LPARAM)&selEnd);
    SetWindowTextW(hEditor, history[historyIndex]);
    SendMessageW(hEditor, EM_SETSEL, selEnd, selEnd);
  }
}

void History::redo() {
  if (canRedo()) {
    DWORD selEnd;
    ++historyIndex;
    SendMessageW(hEditor, EM_GETSEL, (WPARAM)NULL, (LPARAM)&selEnd);
    SetWindowTextW(hEditor, history[historyIndex]);
    SendMessageW(hEditor, EM_SETSEL, selEnd, selEnd);
  }
}

void History::setSaved() {
  if (validHistory) savedIndex = historyIndex;
  SendMessageW(hEditor, EM_SETMODIFY, FALSE, 0);
}

bool History::isSaved() {
  if (validHistory) {
    if (historyIndex == savedIndex) return true;
    if (savedIndex != -1) {
      int editorSize = GetWindowTextLengthW(hEditor) + 1;
      wchar_t *wcEditor = (wchar_t *)malloc(sizeof(wchar_t) * editorSize);
      if (!wcEditor) return false;
      GetWindowTextW(hEditor, wcEditor, editorSize);
      bool result = !wcscmp(history[savedIndex], wcEditor);
      free(wcEditor);
      return result;
    }
    return false;
  } else {
    return SendMessageW(hEditor, EM_GETMODIFY, 0, 0) == FALSE;
  }
}
