#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <string>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace util {
enum newline_t { NEWLINE_CRLF, NEWLINE_LF, NEWLINE_CR };

static inline bool isHex(wchar_t chr) {
  return (chr >= L'0' && chr <= L'9') || (chr >= L'A' && chr <= L'F') ||
         (chr >= L'a' && chr <= L'f');
}

// Shows a message box with TaskDialog or MessageBoxW.
//
// When the system this program is running on supports TaskDialog, and no features that are
// MessageBoxW specific (MB_ABORTRETRYIGNORE, MB_CANCELTRYCONTINUE, MB_HELP, default selection, etc)
// is used, this function uses TaskDialog. In other cases, uses MessageBoxW.
// This function substitutes MB_ICONQUESTION with MB_ICONINFORMATION on TaskDialog, as it doesn't
// support it.
int messageBox(HWND hWnd, HINSTANCE hInst, const wchar_t *lpText, const wchar_t *lpCaption,
               unsigned uType);

// Converts a hex string to a byte sequence. This function allocates `dest` unless it is NULL.
// It's your responsibility to `free()` it.
// Return value of true indicates success, otherwise indicates failure.
bool parseHex(HWND hWnd, HINSTANCE hInst, const wchar_t *hexInput, unsigned char **dest,
              int *length);

// Converts a byte to a hex string. `dest` must have at least 4 elements.
void toHex(unsigned char num, wchar_t *dest);

// Translates newlines and returns the previous newline code.
// `_target`: A `std::wstring` to operate on, `_newLine`: A desired newline code.
enum newline_t convertCRLF(std::wstring &target, enum newline_t newLine);
}  // namespace util
#endif
