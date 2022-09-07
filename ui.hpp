#ifndef UI_HPP_
#define UI_HPP_

#include <windows.h>

#include "bf.hpp"

namespace ui {
enum state_t { STATE_INIT, STATE_RUN, STATE_PAUSE, STATE_FINISH };

extern bool signedness, wrapInt, wrapPtr;
extern int speed, outCharSet, inCharSet;
extern enum bf::noinput_t noInput;
extern HWND hWnd;

// WM_CREATE handler.
void onCreate(HWND _hWnd, HINSTANCE _hInst);

// WM_DESTROY handler.
void onDestroy();

// WM_SIZE handler.
void onSize();

// WM_PAINT handler.
void onPaint();

// WM_ACTIVATE handler.
void onActivate();

// Screen keyboard handler.
void onScreenKeyboard(int _key);

// Sets a UI state.
void setState(enum state_t _state);

// Focus on the editor.
void setFocus();

// Update the menu.
void onInitMenuPopup();

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

// Switch between wordwrap enabled and disabled for edit controls.
void switchWordwrap();

// Check if there will be data loss. If so, suggests to save.
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
