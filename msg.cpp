#include "msg.hpp"

#include "runner.hpp"
#include "ui.hpp"
#include "wproc.hpp"

namespace msg {
static const wchar_t *wcCmdBtn[CMDBTN_LEN] = {L"Run", L"Next", L"Pause", L"End"},
                     *wcScrKB[SCRKBD_LEN] = {L">", L"<", L"+", L"-", L".", L",", L"[", L"]", L"@"};

static HFONT hBtnFont = NULL, hEditFont = NULL;
static int topPadding = 0;
static bool didInit = false;

#ifdef UNDER_CE
static HWND hCmdBar;
#else
static int dpi = 96;
#endif

#ifdef UNDER_CE
// Adjust the coordinate for the current DPI.
static inline int adjust(int coord) { return coord; }
#else
// Adjust the coordinate for the current DPI.
static inline int adjust(int coord) { return coord * dpi / 96; }
#endif

void onActivate(HWND hWnd, UINT state, HWND hWndActDeact, BOOL fMinimized) {
  UNREFERENCED_PARAMETER(hWnd);
  UNREFERENCED_PARAMETER(hWndActDeact);
  UNREFERENCED_PARAMETER(fMinimized);

  if (state == WA_ACTIVE || state == WA_CLICKACTIVE) ui::updateFocus();
}

void onClose(HWND hWnd) {
  if (ui::promptSave()) DestroyWindow(hWnd);
}

void onCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify) {
  UNREFERENCED_PARAMETER(codeNotify);

  ui::updateFocus(hWndCtl);

  switch (id) {
    case IDC_CMDBTN_FIRST:  // Run
      if (!didInit) {
        if (!runner::bfInit()) break;
        didInit = true;
      }

      runner::hThread = myCreateThread(NULL, 0, runner::threadRunner, NULL, 0, NULL);
      if (runner::hThread) {
        SetThreadPriority(runner::hThread, THREAD_PRIORITY_BELOW_NORMAL);
      } else {
        util::messageBox(hWnd, global::hInst, L"Failed to create a runner thread.", L"Internal Error", MB_ICONERROR);
        break;
      }

      SetWindowTextW(global::hMemView, NULL);
      ui::setState(global::STATE_RUN);
      break;

    case IDC_CMDBTN_FIRST + 1: {  // Next
      if (!didInit) {
        if (!runner::bfInit()) break;
        didInit = true;
      }

      Brainfuck::result_t result = runner::bfNext();
      ui::setState(result == Brainfuck::RESULT_FIN || result == Brainfuck::RESULT_ERR ? global::STATE_FINISH
                                                                                      : global::STATE_PAUSE);
      if (result == Brainfuck::RESULT_ERR) {
        util::messageBox(hWnd, global::hInst, runner::bf.getLastError(), L"Brainfuck Error", MB_ICONWARNING);
      }

      unsigned size;
      const unsigned char *memory = runner::bf.getMemory(&size);
      ui::setMemory(memory, size);
      ui::selProg(runner::bf.getProgPtr());
      break;
    }

    case IDC_CMDBTN_FIRST + 2:  // Pause
      if (runner::hThread) runner::ctrlThread = runner::CTRLTHREAD_PAUSE;
      break;

    case IDC_CMDBTN_FIRST + 3:  // End
      if (runner::hThread) {
        runner::ctrlThread = runner::CTRLTHREAD_END;
      } else {
        ui::setState(global::STATE_INIT);
      }
      didInit = false;
      break;

    case IDC_EDITOR:
      if (codeNotify == EN_CHANGE) {
        global::history.add();
        ui::updateTitle();
      }
      break;

    case IDC_INPUT:
      if (codeNotify == EN_CHANGE) {
        global::inputHistory.add();
        ui::updateTitle();
      }
      break;

    case IDM_FILE_NEW:
      ui::openFile(true);
      break;

    case IDM_FILE_OPEN:
      ui::openFile(false);
      break;

    case IDM_FILE_SAVE:
      ui::saveFile(true);
      break;

    case IDM_FILE_SAVE_AS:
      ui::saveFile(false);
      break;

    case IDM_FILE_EXIT:
      SendMessageW(hWnd, WM_CLOSE, 0, 0);
      break;

    case IDM_EDIT_UNDO:
      if (global::hFocused == global::hEditor) {
        global::history.undo();
        ui::updateTitle();
      } else if (global::hFocused == global::hInput) {
        global::inputHistory.undo();
      }
      break;

    case IDM_EDIT_REDO:
      if (global::hFocused == global::hEditor) {
        global::history.redo();
        ui::updateTitle();
      } else if (global::hFocused == global::hInput) {
        global::inputHistory.redo();
      }
      break;

    case IDM_EDIT_CUT:
      SendMessageW(global::hFocused, WM_CUT, 0, 0);
      break;

    case IDM_EDIT_COPY:
      SendMessageW(global::hFocused, WM_COPY, 0, 0);
      break;

    case IDM_EDIT_PASTE:
      SendMessageW(global::hFocused, WM_PASTE, 0, 0);
      break;

    case IDM_EDIT_SELALL:
      SendMessageW(global::hFocused, EM_SETSEL, 0, -1);
      break;

    case IDM_BF_MEMTYPE_SIGNED:
      global::signedness = true;
      break;

    case IDM_BF_MEMTYPE_UNSIGNED:
      global::signedness = false;
      break;

    case IDM_BF_OUTPUT_UTF8:
      global::outCharSet = IDM_BF_OUTPUT_UTF8;
      break;

    case IDM_BF_OUTPUT_SJIS:
      global::outCharSet = IDM_BF_OUTPUT_SJIS;
      break;

    case IDM_BF_OUTPUT_HEX:
      global::outCharSet = IDM_BF_OUTPUT_HEX;
      break;

    case IDM_BF_INPUT_UTF8:
      global::inCharSet = IDM_BF_INPUT_UTF8;
      break;

    case IDM_BF_INPUT_SJIS:
      global::inCharSet = IDM_BF_INPUT_SJIS;
      break;

    case IDM_BF_INPUT_HEX:
      global::inCharSet = IDM_BF_INPUT_HEX;
      break;

    case IDM_BF_NOINPUT_ERROR:
      global::noInput = Brainfuck::NOINPUT_ERROR;
      break;

    case IDM_BF_NOINPUT_ZERO:
      global::noInput = Brainfuck::NOINPUT_ZERO;
      break;

    case IDM_BF_NOINPUT_FF:
      global::noInput = Brainfuck::NOINPUT_FF;
      break;

    case IDM_BF_INTOVF_ERROR:
      global::wrapInt = false;
      break;

    case IDM_BF_INTOVF_WRAPAROUND:
      global::wrapInt = true;
      break;

    case IDM_BF_BREAKPOINT:
      global::breakpoint = !global::breakpoint;
      runner::bf.setBehavior(global::noInput, global::wrapInt, global::signedness, global::breakpoint);
      break;

    case IDM_OPT_SPEED_FASTEST:
      global::speed = 0;
      break;

    case IDM_OPT_SPEED_1MS:
      global::speed = 1;
      break;

    case IDM_OPT_SPEED_10MS:
      global::speed = 10;
      break;

    case IDM_OPT_SPEED_100MS:
      global::speed = 100;
      break;

    case IDM_OPT_MEMVIEW:
      if (DialogBoxW(global::hInst, L"memviewopt", hWnd, wproc::memViewProc) == IDOK &&
          global::state != global::STATE_INIT) {
        unsigned size;
        const unsigned char *memory = runner::bf.getMemory(&size);
        ui::setMemory(memory, size);
      }
      break;

    case IDM_OPT_TRACK:
      global::debug = !global::debug;
      break;

    case IDM_OPT_HLTPROG:
      ui::selProg(runner::bf.getProgPtr());
      break;

    case IDM_OPT_HLTMEM:
      ui::selMemView(runner::bf.getMemPtr());
      break;

    case IDM_OPT_DARK:
      ui::switchTheme();
      break;

    case IDM_OPT_LAYOUT:
      ui::switchLayout();
      break;

    case IDM_OPT_FONT:
      ui::chooseFont();
      break;

    case IDM_OPT_WORDWRAP:
      ui::switchWordwrap();
      break;

    case IDM_ABOUT:
      util::messageBox(hWnd, global::hInst,
            APP_NAME L" version " APP_VERSION L"\r\n"
            APP_DESCRIPTION L"\r\n\r\n"
            APP_COPYRIGHT L"\r\n"
            L"Licensed under the MIT License.",
            L"About this software",
            0
          );
      break;

    default:
      if (id >= IDC_SCRKBD_FIRST && id < IDC_SCRKBD_FIRST + SCRKBD_LEN && global::hFocused == global::hEditor) {
        SendMessageW(global::hEditor, EM_REPLACESEL, 0, (WPARAM)wcScrKB[id - IDC_SCRKBD_FIRST]);
      }
  }
}

BOOL onCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct) {
  UNREFERENCED_PARAMETER(lpCreateStruct);

  size_t i;
  global::hWnd = hWnd;

#ifdef UNDER_CE
  wchar_t wcMenu[] = L"menu";  // CommandBar_InsertMenubarEx requires non-const value.
  InitCommonControls();
  hCmdBar = CommandBar_Create(global::hInst, hWnd, 1);
  CommandBar_InsertMenubarEx(hCmdBar, global::hInst, wcMenu, 0);
  CommandBar_Show(hCmdBar, TRUE);
  topPadding = CommandBar_Height(hCmdBar);
  global::hMenu = CommandBar_GetMenu(hCmdBar, 0);
#else
  global::hMenu = LoadMenu(global::hInst, L"menu");
  SetMenu(hWnd, global::hMenu);
#endif

  // Program editor
  global::hEditor =
      CreateWindowExW(0, L"EDIT", L"",
                      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL |
                          (global::wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL) | ES_NOHIDESEL,
                      0, 0, 0, 0, hWnd, (HMENU)IDC_EDITOR, global::hInst, NULL);
  SendMessageW(global::hEditor, EM_SETLIMITTEXT, (WPARAM)-1, 0);
  mySetWindowLongW(global::hEditor, GWL_USERDATA, mySetWindowLongW(global::hEditor, GWL_WNDPROC, wproc::editorProc));

  // Program input
  global::hInput = CreateWindowExW(0, L"EDIT", L"",
                                   WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                                       WS_VSCROLL | (global::wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL) | ES_NOHIDESEL,
                                   0, 0, 0, 0, hWnd, (HMENU)IDC_INPUT, global::hInst, NULL);
  SendMessageW(global::hInput, EM_SETLIMITTEXT, (WPARAM)-1, 0);
  mySetWindowLongW(global::hInput, GWL_USERDATA, mySetWindowLongW(global::hInput, GWL_WNDPROC, wproc::inputProc));
  global::inputHistory.reset(global::hInput);

  // Program output
  global::hOutput =
      CreateWindowExW(0, L"EDIT", L"",
                      WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL |
                          WS_VSCROLL | (global::wordwrap ? 0 : ES_AUTOHSCROLL | WS_HSCROLL) | ES_NOHIDESEL,
                      0, 0, 0, 0, hWnd, (HMENU)IDC_OUTPUT, global::hInst, NULL);
  SendMessageW(global::hOutput, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  // Memory view
  global::hMemView = CreateWindowExW(0, L"EDIT", L"",
                                     WS_CHILD | WS_VISIBLE | WS_BORDER | ES_LEFT | ES_MULTILINE | ES_READONLY |
                                         ES_AUTOVSCROLL | WS_VSCROLL | ES_NOHIDESEL,
                                     0, 0, 0, 0, hWnd, (HMENU)IDC_MEMVIEW, global::hInst, NULL);
  SendMessageW(global::hMemView, EM_SETLIMITTEXT, (WPARAM)-1, 0);

  // Command button
  for (i = 0; i < CMDBTN_LEN; ++i) {
    global::hCmdBtn[i] = CreateWindowExW(0, L"BUTTON", wcCmdBtn[i],
                                         WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | (i == 2 || i == 3 ? WS_DISABLED : 0),
                                         0, 0, 0, 0, hWnd, (HMENU)(IDC_CMDBTN_FIRST + i), global::hInst, NULL);
  }

  // Screen keyboard
  for (i = 0; i < SCRKBD_LEN; ++i) {
    global::hScrKB[i] = CreateWindowExW(0, L"BUTTON", wcScrKB[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 0, 0, 0, 0,
                                        hWnd, (HMENU)(IDC_SCRKBD_FIRST + i), global::hInst, NULL);
  }

  global::hFocused = global::hEditor;

  // Default configurations for the editor font.
  // We create the actual font object on onSize function.
  ZeroMemory(&global::editFont, sizeof(global::editFont));
  global::editFont.lfCharSet = DEFAULT_CHARSET;
  global::editFont.lfQuality = ANTIALIASED_QUALITY;
  global::editFont.lfHeight = -15;  // Represents font size 11 in 96 DPI.
#ifdef UNDER_CE
  // Sets a pre-installed font on Windows CE, as it doesn't have "MS Shell Dlg".
  lstrcpyW(global::editFont.lfFaceName, L"Tahoma");
#else
  // Sets a logical font face name for localization.
  // It maps to a default shell font associated with the current culture/locale.
  lstrcpyW(global::editFont.lfFaceName, L"MS Shell Dlg");

  // Obtains the "system DPI" value. We use this as the fallback value on older Windows versions and to calculate the
  // appropriate font height value for ChooseFontW.
  HDC hDC = GetDC(hWnd);
  global::sysDPI = GetDeviceCaps(hDC, LOGPIXELSX);
  ReleaseDC(hWnd, hDC);

  // Tries to load the GetDpiForMonitor API. Use of the full path improves security. We avoid a direct call to keep this
  // program compatible with Windows 7 and earlier.
  //
  // Microsoft recommends the use of GetDpiForWindow API instead of this API according to their documentation. However,
  // it requires Windows 10 1607 or later, which makes this compatibility keeping code more complicated, and
  // GetDpiForMonitor API still works for programs that only use the process-wide DPI awareness. Here, as we only use
  // the process-wide DPI awareness, we are going to use GetDpiForMonitor API.
  //
  // References:
  // https://learn.microsoft.com/en-us/windows/win32/api/shellscalingapi/nf-shellscalingapi-getdpiformonitor
  // https://mariusbancila.ro/blog/2021/05/19/how-to-build-high-dpi-aware-native-desktop-applications/
  GetDpiForMonitor_t getDpiForMonitor = NULL;
  HMODULE dll = LoadLibraryW(L"C:\\Windows\\System32\\Shcore.dll");
  if (dll) getDpiForMonitor = (GetDpiForMonitor_t)(void *)GetProcAddress(dll, "GetDpiForMonitor");

  // Tests whether it successfully got the GetDpiForMonitor API.
  if (getDpiForMonitor) {  // It got (the system is presumably Windows 8.1 or later).
    unsigned tmpX, tmpY;
    getDpiForMonitor(MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST), MDT_EFFECTIVE_DPI, &tmpX, &tmpY);
    dpi = tmpX;
  } else {  // It failed (the system is presumably older than Windows 8.1).
    dpi = global::sysDPI;
  }
  if (dll) FreeLibrary(dll);

  // Adjusts the window size according to the DPI value.
  SetWindowPos(hWnd, NULL, 0, 0, adjust(480), adjust(320), SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
#endif

  ui::openFile(!global::cmdLine[0], global::cmdLine);

  return TRUE;
}

void onDestroy() {
  DeleteObject(hBtnFont);
  DeleteObject(hEditFont);
}

void onInitMenuPopup(HWND hWnd, HMENU hMenu, UINT item, BOOL fSystemMenu) {
  UNREFERENCED_PARAMETER(hWnd);
  UNREFERENCED_PARAMETER(item);
  UNREFERENCED_PARAMETER(fSystemMenu);

  // Brainfuck -> Memory Type
  CheckMenuRadioItem(hMenu, IDM_BF_MEMTYPE_SIGNED, IDM_BF_MEMTYPE_UNSIGNED,
                     global::signedness ? IDM_BF_MEMTYPE_SIGNED : IDM_BF_MEMTYPE_UNSIGNED, MF_BYCOMMAND);

  // Brainfuck -> Output Charset
  CheckMenuRadioItem(hMenu, IDM_BF_OUTPUT_UTF8, IDM_BF_OUTPUT_HEX, global::outCharSet, MF_BYCOMMAND);

  // Brainfuck -> Input Charset
  CheckMenuRadioItem(hMenu, IDM_BF_INPUT_UTF8, IDM_BF_INPUT_HEX, global::inCharSet, MF_BYCOMMAND);

  // Brainfuck -> Input Instruction
  CheckMenuRadioItem(hMenu, IDM_BF_NOINPUT_ERROR, IDM_BF_NOINPUT_FF, IDM_BF_NOINPUT_ERROR + global::noInput,
                     MF_BYCOMMAND);

  // Brainfuck -> Integer Overflow
  CheckMenuRadioItem(hMenu, IDM_BF_INTOVF_ERROR, IDM_BF_INTOVF_WRAPAROUND,
                     global::wrapInt ? IDM_BF_INTOVF_WRAPAROUND : IDM_BF_INTOVF_ERROR, MF_BYCOMMAND);

  // Brainfuck -> Breakpoint
  CheckMenuItem(hMenu, IDM_BF_BREAKPOINT, MF_BYCOMMAND | global::breakpoint ? MF_CHECKED : MF_UNCHECKED);

  // Options -> Speed
  CheckMenuRadioItem(hMenu, IDM_OPT_SPEED_FASTEST, IDM_OPT_SPEED_100MS,
                     global::speed == 0    ? IDM_OPT_SPEED_FASTEST
                     : global::speed == 1  ? IDM_OPT_SPEED_1MS
                     : global::speed == 10 ? IDM_OPT_SPEED_10MS
                                           : IDM_OPT_SPEED_100MS,
                     MF_BYCOMMAND);

  // Options -> Debug
  CheckMenuItem(hMenu, IDM_OPT_TRACK, MF_BYCOMMAND | global::debug ? MF_CHECKED : MF_UNCHECKED);

  // Options -> Word Wrap
  CheckMenuItem(hMenu, IDM_OPT_WORDWRAP, MF_BYCOMMAND | global::wordwrap ? MF_CHECKED : MF_UNCHECKED);

  if (global::state == global::STATE_INIT) {
    ui::enableMenus(IDM_FILE_NEW, true);
    ui::enableMenus(IDM_FILE_OPEN, true);
    ui::enableMenus(IDM_EDIT_UNDO, global::hFocused == global::hEditor  ? global::history.canUndo()
                                   : global::hFocused == global::hInput ? global::inputHistory.canUndo()
                                                                        : false);
    ui::enableMenus(IDM_EDIT_REDO, global::hFocused == global::hEditor  ? global::history.canRedo()
                                   : global::hFocused == global::hInput ? global::inputHistory.canRedo()
                                                                        : false);
    ui::enableMenus(IDM_BF_MEMTYPE_UNSIGNED, true);
    ui::enableMenus(IDM_BF_OUTPUT_HEX, true);
    ui::enableMenus(IDM_BF_INPUT_HEX, true);
    ui::enableMenus(IDM_BF_NOINPUT_FF, true);
    ui::enableMenus(IDM_BF_INTOVF_WRAPAROUND, true);
    ui::enableMenus(IDM_BF_BREAKPOINT, true);
    ui::enableMenus(IDM_OPT_SPEED_100MS, true);
    ui::enableMenus(IDM_OPT_MEMVIEW, true);
    ui::enableMenus(IDM_OPT_TRACK, true);
    ui::enableMenus(IDM_OPT_HLTPROG, false);
    ui::enableMenus(IDM_OPT_HLTMEM, false);
  } else if (global::state == global::STATE_RUN) {
    ui::enableMenus(IDM_FILE_NEW, false);
    ui::enableMenus(IDM_FILE_OPEN, false);
    ui::enableMenus(IDM_EDIT_UNDO, false);
    ui::enableMenus(IDM_EDIT_REDO, false);
    ui::enableMenus(IDM_BF_MEMTYPE_UNSIGNED, false);
    ui::enableMenus(IDM_BF_OUTPUT_HEX, false);
    ui::enableMenus(IDM_BF_INPUT_HEX, false);
    ui::enableMenus(IDM_BF_NOINPUT_FF, false);
    ui::enableMenus(IDM_BF_INTOVF_WRAPAROUND, false);
    ui::enableMenus(IDM_BF_BREAKPOINT, false);
    ui::enableMenus(IDM_OPT_SPEED_100MS, false);
    ui::enableMenus(IDM_OPT_MEMVIEW, false);
    ui::enableMenus(IDM_OPT_TRACK, false);
    ui::enableMenus(IDM_OPT_HLTPROG, false);
    ui::enableMenus(IDM_OPT_HLTMEM, false);
  } else if (global::state == global::STATE_PAUSE || global::state == global::STATE_FINISH) {
    ui::enableMenus(IDM_FILE_NEW, false);
    ui::enableMenus(IDM_FILE_OPEN, false);
    ui::enableMenus(IDM_EDIT_UNDO, false);
    ui::enableMenus(IDM_EDIT_REDO, false);
    ui::enableMenus(IDM_BF_MEMTYPE_UNSIGNED, false);
    ui::enableMenus(IDM_BF_OUTPUT_HEX, false);
    ui::enableMenus(IDM_BF_INPUT_HEX, false);
    ui::enableMenus(IDM_BF_NOINPUT_FF, false);
    ui::enableMenus(IDM_BF_INTOVF_WRAPAROUND, false);
    ui::enableMenus(IDM_BF_BREAKPOINT, true);
    ui::enableMenus(IDM_OPT_SPEED_100MS, true);
    ui::enableMenus(IDM_OPT_MEMVIEW, true);
    ui::enableMenus(IDM_OPT_TRACK, true);
    ui::enableMenus(IDM_OPT_HLTPROG, true);
    ui::enableMenus(IDM_OPT_HLTMEM, true);
  }
}

void onSize(HWND hWnd, UINT state, int cx, int cy) {
  UNREFERENCED_PARAMETER(state);
  UNREFERENCED_PARAMETER(cx);
  UNREFERENCED_PARAMETER(cy);

  onSize(hWnd);
}

void onSize(HWND hWnd) {
  RECT rect;

#ifdef UNDER_CE
  // Manually forces the minimum window size as Windows CE doesn't support WM_GETMINMAXINFO.
  // Seemingly, Windows CE doesn't re-send WM_SIZE on SetWindowPos calls inside a WM_SIZE handler.
  GetWindowRect(hWnd, &rect);
  if (rect.right - rect.left < 480 || rect.bottom - rect.top < 320) {
    SetWindowPos(hWnd, NULL, 0, 0, mymax(480, rect.right - rect.left), mymax(320, rect.bottom - rect.top),
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
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
  lstrcpyW(rLogfont.lfFaceName, global::editFont.lfFaceName);  // Syncs with editors.
  HFONT newBtnFont = CreateFontIndirectW(&rLogfont);

  // Editor/Output font
  memcpy(&rLogfont, &global::editFont, sizeof(LOGFONTW));
  rLogfont.lfHeight = adjust(global::editFont.lfHeight);
  HFONT newEditFont = CreateFontIndirectW(&rLogfont);

#ifdef UNDER_CE
  // We must "move" the command bar to prevent a glitch.
  MoveWindow(hCmdBar, 0, 0, 0, 0, TRUE);
#endif

  // Moves and resizes controls, and applies the newly created fonts for them.
  int curX = 0;
  for (i = 0; i < CMDBTN_LEN; ++i) {
    MoveWindow(global::hCmdBtn[i], curX, topPadding, adjust(46), adjust(32), TRUE);
    SendMessageW(global::hCmdBtn[i], WM_SETFONT, (WPARAM)newBtnFont, MAKELPARAM(TRUE, 0));
    curX += adjust(46);
  }
  for (i = 0; i < SCRKBD_LEN; ++i) {
    MoveWindow(global::hScrKB[i], curX, topPadding, adjust(30), adjust(32), TRUE);
    SendMessageW(global::hScrKB[i], WM_SETFONT, (WPARAM)newBtnFont, MAKELPARAM(TRUE, 0));
    curX += adjust(30);
  }

  int topEditor = topPadding + adjust(32);
  GetClientRect(hWnd, &rect);
  int scrX = rect.right, scrY = rect.bottom;
  int halfY = (scrY - topEditor) / 2, centerY = (scrY + topEditor) / 2;
  if (global::horizontal) {
    MoveWindow(global::hEditor, 0, topEditor, scrX / 3, scrY - topEditor, TRUE);
    MoveWindow(global::hInput, scrX / 3, topEditor, scrX / 3, halfY, TRUE);
    MoveWindow(global::hOutput, scrX / 3, halfY + topEditor, scrX / 3, scrY - halfY - topEditor, TRUE);
    MoveWindow(global::hMemView, scrX * 2 / 3, topEditor, scrX - scrX * 2 / 3, scrY - topEditor, TRUE);
  } else {
    MoveWindow(global::hEditor, 0, topEditor, scrX, halfY, TRUE);
    MoveWindow(global::hInput, 0, centerY, scrX / 2, halfY / 2, TRUE);
    MoveWindow(global::hOutput, scrX / 2, centerY, scrX - scrX / 2, halfY / 2, TRUE);
    MoveWindow(global::hMemView, 0, centerY + halfY / 2, scrX, scrY - centerY - halfY / 2, TRUE);
  }
  SendMessageW(global::hEditor, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(TRUE, 0));
  SendMessageW(global::hInput, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(TRUE, 0));
  SendMessageW(global::hOutput, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(TRUE, 0));
  SendMessageW(global::hMemView, WM_SETFONT, (WPARAM)newEditFont, MAKELPARAM(TRUE, 0));

  if (hBtnFont) DeleteObject(hBtnFont);
  if (hEditFont) DeleteObject(hEditFont);
  hBtnFont = newBtnFont;
  hEditFont = newEditFont;
  InvalidateRect(hWnd, NULL, FALSE);
}

#ifndef UNDER_CE
void onDropFiles(HWND hWnd, HDROP hDrop) {
  UNREFERENCED_PARAMETER(hWnd);

  wchar_t wcFileName[MAX_PATH];
  DragQueryFileW(hDrop, 0, wcFileName, MAX_PATH);
  DragFinish(hDrop);
  ui::openFile(false, wcFileName);
}

void onGetMinMaxInfo(HWND hWnd, LPMINMAXINFO lpMinMaxInfo) {
  UNREFERENCED_PARAMETER(hWnd);

  lpMinMaxInfo->ptMinTrackSize.x = adjust(480);
  lpMinMaxInfo->ptMinTrackSize.y = adjust(320);
}

void onDPIChanged(HWND hWnd, int dpi, const RECT *rect) {
  msg::dpi = dpi;
  MoveWindow(hWnd, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, FALSE);
}
#endif
}  // namespace msg
