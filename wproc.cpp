#include "wproc.hpp"

#include <windowsx.h>

#include "msg.hpp"
#include "runner.hpp"

namespace wproc {
LRESULT CALLBACK wndProc(HWND hWnd, unsigned int uMsg, WPARAM wParam, LPARAM lParam) {
  static HBRUSH BGDark = CreateSolidBrush(0x3f3936);  // Background brush for the dark theme.

  switch (uMsg) {
    HANDLE_MSG(hWnd, WM_ACTIVATE, msg::onActivate);
    HANDLE_MSG(hWnd, WM_CLOSE, msg::onClose);
    HANDLE_MSG(hWnd, WM_COMMAND, msg::onCommand);
    HANDLE_MSG(hWnd, WM_CREATE, msg::onCreate);
    HANDLE_MSG(hWnd, WM_INITMENUPOPUP, msg::onInitMenuPopup);
    HANDLE_MSG(hWnd, WM_SIZE, msg::onSize);

#ifndef UNDER_CE
    HANDLE_MSG(hWnd, WM_DROPFILES, msg::onDropFiles);
    HANDLE_MSG(hWnd, WM_GETMINMAXINFO, msg::onGetMinMaxInfo);

    case 0x2e0:  // WM_DPICHANGED
      msg::onDPIChanged(hWnd, HIWORD(wParam), (const RECT *)lParam);
      return 0;
#endif

    case WM_CTLCOLOREDIT:
      if (global::dark) {
        SetTextColor((HDC)wParam, 0x00ff00);
        SetBkColor((HDC)wParam, 0x3f3936);
        return (LRESULT)BGDark;
      } else {
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
      }

    case WM_CTLCOLORSTATIC:
      if (global::dark) {
        SetTextColor((HDC)wParam, 0x00ff00);
        SetBkColor((HDC)wParam, 0x000000);
        return (LRESULT)GetStockObject(BLACK_BRUSH);
      } else {
        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
      }

    case WM_DESTROY:
      msg::onDestroy();
      DeleteObject(BGDark);
      PostQuitMessage(0);
      return 0;

    case WM_APP_THREADEND:
      WaitForSingleObject(runner::hThread, INFINITE);
      CloseHandle(runner::hThread);
      runner::hThread = NULL;
      runner::ctrlThread = runner::CTRLTHREAD_RUN;
      return 0;
  }

  return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK editorProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam) {
  static WNDPROC prevWndProc = (WNDPROC)myGetWindowLongW(hWnd, GWL_USERDATA);

  switch (uMsg) {
    case WM_CHAR: {
      if (!global::validHistory) break;

      int editorSize = GetWindowTextLengthW(global::hEditor) + 1;
      wchar_t *wcEditor = (wchar_t *)malloc(sizeof(wchar_t) * editorSize);
      if (!wcEditor) {
        util::messageBox(hWnd, global::hInst, L"Memory allocation failed.", L"Internal Error",
                         MB_ICONWARNING);
        while (!global::history.empty()) {
          free(global::history.back());
          global::history.pop_back();
        }
        global::historyIndex = 0;
        global::savedIndex = -1;
        global::validHistory = false;
        break;
      }
      GetWindowTextW(global::hEditor, wcEditor, editorSize);

      if (!global::history.empty() && !wcscmp(global::history[global::historyIndex], wcEditor)) {
        free(wcEditor);
        break;
      }

      int toRemove = (int)global::history.size() - global::historyIndex - 1, i = 0;
      for (; i < toRemove; ++i) {
        free(global::history.back());
        global::history.pop_back();
      }

      if (global::history.size() >= MAX_HISTORY) {
        free(global::history.front());
        global::history.pop_front();
        --global::historyIndex;
        if (global::savedIndex >= 0) --global::savedIndex;
      }

      global::history.push_back(wcEditor);
      ++global::historyIndex;
      break;
    }

    case WM_DESTROY:
      mySetWindowLongW(hWnd, GWL_WNDPROC, prevWndProc);
      return 0;
  }

  return CallWindowProcW(prevWndProc, hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK memViewDlgEditor(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam) {
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

INT_PTR CALLBACK memViewProc(HWND hDlg, unsigned uMsg, WPARAM wParam, LPARAM lParam) {
  UNREFERENCED_PARAMETER(lParam);

  switch (uMsg) {
    case WM_INITDIALOG: {
      // Puts the dialog at the center of the parent window.
      RECT wndSize, wndRect, dlgRect;
      GetWindowRect(hDlg, &dlgRect);
      GetWindowRect(global::hWnd, &wndRect);
      GetClientRect(global::hWnd, &wndSize);
      int newPosX = wndRect.left + (wndSize.right - (dlgRect.right - dlgRect.left)) / 2,
          newPosY = wndRect.top + (wndSize.bottom - (dlgRect.bottom - dlgRect.top)) / 2;
      if (newPosX < 0) newPosX = 0;
      if (newPosY < 0) newPosY = 0;
      SetWindowPos(hDlg, NULL, newPosX, newPosY, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

      wchar_t editBuf[10];
      HWND hEdit = GetDlgItem(hDlg, 3);
      wsprintfW(editBuf, L"%u",
                global::memViewStart <= 999999999 ? global::memViewStart : 999999999);
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
            util::messageBox(hDlg, global::hInst, L"Invalid input.", L"Error", MB_ICONWARNING);
          } else {
            global::memViewStart = temp;
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
}  // namespace wproc
