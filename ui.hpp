#ifndef UI_HPP_
#define UI_HPP_

#include "main.hpp"

namespace ui {
// Enables/Disables menu items from the smaller nearest 10 multiple to `_endID`.
void enableMenus(unsigned endID, bool enable);

// Sets a UI state.
void setState(enum global::state_t state, bool force = false);

// Focuses on a recently focused edit control if a non-edit control is given.
// Updates internal information if an edit control is given.
void updateFocus(HWND hWndCtl = NULL);

// Sets a selection on the editor.
void selProg(unsigned progPtr);

// Sets a selection on the memory view.
void selMemView(unsigned memPtr);

// Outputs an null-terminated string to the output box.
void appendOutput(const wchar_t *str);

// Sets memory.
void setMemory(const unsigned char *memory, int size = 0);

// Switches between wordwrap enabled and disabled for edit controls.
void switchWordwrap();

// Switches between dark/light theme.
void switchTheme();

// Switches between horizontal/portrait layout.
void switchLayout();

// Sets the title of the main window.
void updateTitle();

// Opens a font dialog and applies it on editors.
void chooseFont();

// Checks if there will be data loss. If so, suggests to save.
// `true` means you can continue a data losing action.
bool promptSave();

// Opens a file to the editor. Give it `NULL` or empty string to create new.
// Nothing will be changed when canceled by the user.
void openFile(bool newFile, const wchar_t *fileName = NULL);

// Saves the editor content to a file.
// Return value of `true` means successfully saved and `false` means not saved.
bool saveFile(bool isOverwrite);
}  // namespace ui

#endif
