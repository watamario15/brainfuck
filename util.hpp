#ifndef UTIL_HPP_
#define UTIL_HPP_

#include <string>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace util {
enum newline_t { NEWLINE_CRLF, NEWLINE_LF, NEWLINE_CR };

// Converts a hex string to a byte sequence. This function allocates `dest` unless it is NULL.
// It's your responsibility to `free()` it.
// Return value of true indicates success, otherwise indicates failure.
extern bool parseHex(HWND hWnd, const wchar_t *hexInput, unsigned char **dest);

// Converts a byte to a hex string. `dest` must have at least 4 elements.
extern void toHex(unsigned char num, wchar_t *dest);

// Translates newlines and returns the previous newline code.
// `_target`: A `std::wstring` to operate on, `_newLine`: A desired newline code.
enum newline_t convertCRLF(std::wstring &_target, enum newline_t _newLine);
}  // namespace util
#endif
