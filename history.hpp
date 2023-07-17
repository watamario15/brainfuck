#ifndef HISTORY_HPP_
#define HISTORY_HPP_

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <deque>

#define MAX_HISTORY 128

class History {
 public:
  History() { reset(NULL); }

  ~History() {
    while (!history.empty()) {
      free(history.back());
      history.pop_back();
    }
  }

  // Resets the undo/redo history and sets `str` as the first entry.
  // `hEditor` is the handle to a edit control which this module tracks the history of.
  // If `str` is NULL, the first entry will be an empty string.
  void reset(HWND hEditor, const wchar_t *str = NULL);

  // Adds the current edit control text to the undo/redo history.
  // Redo history will be cleared.
  void add();

  // Changes back the edit control text to the previous state.
  void undo();

  // Changes the edit control text to the next state.
  void redo();

  bool canUndo() { return validHistory && historyIndex > 0; }

  bool canRedo() { return validHistory && historyIndex < (int)history.size() - 1; }

  // Marks the current edit control text as saved.
  void setSaved();

  // Returns whether the current edit control text is saved.
  bool isSaved();

  // Changes the edit control which this module tracks the history of, without modifing the history.
  void changeEditor(HWND hEditor) { this->hEditor = hEditor; }

 private:
  int historyIndex;               // Index to the current state in the undo/redo history.
  int savedIndex;                 // Index to the saved state in the undo/redo history.
  bool validHistory;              // Whether the undo/redo history is valid.
  HWND hEditor;                   // Handle to the edit control.
  std::deque<wchar_t *> history;  // Undo/Redo history.
};

#endif
