#ifndef UTIL_HPP_
#define UTIL_HPP_

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string>

namespace util {
enum newline_t { NEWLINE_CRLF, NEWLINE_LF, NEWLINE_CR };

class UTF8Parser {
 public:
  UTF8Parser() : state(0), index(0){};

  // Resets and returns the remaining bytes if any, all transformed to "?".
  // Returns NULL when no bytes are left.
  const unsigned char *flush();

  // Adds a byte to the internal buffer until it becomes one token.
  // Returns NULL when more bytes are required, and returns a token when completed.
  // Note that this function can return multiple "?" characters when it is given a corrupted UTF-8
  // sequence.
  const unsigned char *add(char chr);

 private:
  int state, index;
  unsigned char token[10];
};

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
