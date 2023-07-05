#ifndef WPROC_HPP_
#define WPROC_HPP_

#include "main.hpp"

namespace wproc {
// Window procedure for the main window.
LRESULT CALLBACK wndProc(HWND hWnd, unsigned int uMsg, WPARAM wParam, LPARAM lParam);

// Hook window procedure for the program editor to support the undo feature.
LRESULT CALLBACK editorProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);

// Hook window procedure for the input editor to support the undo feature.
LRESULT CALLBACK inputProc(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);

// Hook window procedure for the edit control in the memory view options dialog.
// This procedure translates top row character keys to numbers according to the keyboard layout of SHARP Brain devices.
LRESULT CALLBACK memViewDlgEditor(HWND hWnd, unsigned uMsg, WPARAM wParam, LPARAM lParam);

// Window procedure for the memory view options dialog.
INT_PTR CALLBACK memViewProc(HWND hDlg, unsigned uMsg, WPARAM wParam, LPARAM lParam);
}  // namespace wproc

#endif
