#ifndef HOOK_HPP_
#define HOOK_HPP_

#include "main.hpp"

namespace wproc {
// Window procedure for the main window.
LRESULT CALLBACK wndProc(HWND hWnd, unsigned int uMsg, WPARAM wParam, LPARAM lParam);

// Hook window procedure for the program editor that manages the undo/redo history.
LRESULT CALLBACK editorProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);

// Hook window procedure for the edit control in the memory view options dialog.
// This procedure translates top row character keys to numbers according to the keyboard layout of
// SHARP Brain devices.
LRESULT CALLBACK memViewDlgEditor(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);

// Window procedure for the memory view options dialog.
INT_PTR CALLBACK memViewProc(HWND hDlg, unsigned uMsg, WPARAM wParam, LPARAM lParam);
}  // namespace hook

#endif
