#ifndef MSG_HPP_
#define MSG_HPP_

#include "main.hpp"

namespace msg {
extern void onActivate(HWND hWnd, UINT state, HWND hWndActDeact, BOOL fMinimized);
extern void onClose(HWND hWnd);
extern void onCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify);
extern BOOL onCreate(HWND hWnd, CREATESTRUCTW *lpCreateStruct);
extern void onDestroy();
extern void onInitMenuPopup(HWND hWnd, HMENU hMenu, UINT item, BOOL fSystemMenu);
extern void onSize(HWND hWnd, UINT state, int cx, int cy);
extern void onSize(HWND hWnd);
#ifndef UNDER_CE
extern void onDropFiles(HWND hWnd, HDROP hdrop);
extern void onGetMinMaxInfo(HWND hWnd, MINMAXINFO *lpMinMaxInfo);
extern void onDPIChanged(HWND hWnd, int dpi, const RECT *rect);
#endif
}  // namespace msg

#endif
