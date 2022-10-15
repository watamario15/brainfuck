#ifndef UI_HPP_
#define UI_HPP_

#include <windows.h>

#include "bf.hpp"

namespace ui {
enum state_t { STATE_INIT, STATE_RUN, STATE_PAUSE, STATE_FINISH };

extern enum state_t state;
extern bool signedness, wrapInt, breakpoint, debug, dark;
extern int speed, outCharSet, inCharSet;
extern enum Brainfuck::noinput_t noInput;
extern HWND hWnd;
extern HINSTANCE hInst;

// WM_CREATE handler.
void onCreate(HWND _hWnd, HINSTANCE _hInst);

// WM_DESTROY handler.
void onDestroy();

// WM_SIZE handler.
void onSize();

// Updates the menu.
void onInitMenuPopup();

#ifndef UNDER_CE
// WM_DROPFILES handler.
void onDropFiles(HDROP hDrop);
#endif

// Screen keyboard handler.
void onScreenKeyboard(int _key);

// Sets a UI state.
void setState(enum state_t _state, bool _force = false);

// Focuses on a recently focused edit control if a non-edit ID is given.
// Updates internal information if an edit control ID is given.
void updateFocus(int _id = -1);

// Cut
void cut();

// Copy
void copy();

// Paste
void paste();

// Select All
void selAll();

// Sets a selection on the editor.
void selProg(unsigned int _progPtr);

// Sets a selection on the memory view.
void selMemView(unsigned int _memPtr);

// Memory view settings dialog.
INT_PTR CALLBACK memViewProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Retrieves the editor content to the returned buffer.
// Previously returned pointer gets invalidated on each call.
wchar_t *getEditor();

// Retrieves the input content to the returned buffer.
// Previously returned pointer gets invalidated on each call.
wchar_t *getInput();

// Clears the output box.
void clearOutput();

// Sets an null-terminated string to the output box.
void setOutput(const wchar_t *_str);

// Outputs an null-terminated string to the output box.
void appendOutput(const wchar_t *_str);

// Sets memory. Clears the memory view when NULL is given.
void setMemory(const std::vector<unsigned char> *memory);

// Switches between wordwrap enabled and disabled for edit controls.
void switchWordwrap();

// Switches between dark/light theme.
void switchTheme();

// Checks if there will be data loss. If so, suggests to save.
// `true` means you can continue a data losing action.
bool promptSave();

// Opens a file to the editor. Give it `NULL` or empty string to create new.
// Nothing will be changed when canceled by the user.
void openFile(bool _newFile, const wchar_t *_fileName = NULL);

// Saves the editor content to a file.
// Return value of `true` means successfully saved and `false` means not saved.
bool saveFile(bool _isOverwrite);
}  // namespace ui
#endif
