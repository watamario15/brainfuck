#include "util.hpp"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <string>
#include "ui.hpp"

namespace util {
const unsigned char *UTF8Parser::flush() {
  if (index == 0) return NULL;
  for (int i = 0; i < index; ++i) token[i] = '?';
  state = index = 0;
  return token;
}

const unsigned char *UTF8Parser::add(char chr) {
  if (state == 0) {  // Reading the first character of a token.
    memset(token, 0, sizeof(token));

    // Got a later byte character, which is unexpected here.
    if ((chr & 0xC0) == 0x80) {
      token[0] = '?';
      return token;
    }

    token[0] = chr;

    if (!(chr & 0x80)) return token;  // ASCII

    char tmp = chr << 2;
    for (int i = 1; true; ++i) {
      if (!(tmp & 0x80)) {
        state = i;
        break;
      }
      if (i == 6) {
        state = 7;
        break;
      }
      tmp <<= 1;
    }
    ++index;
  } else {  // Reading a later byte character of a token.
    // Got a first byte character, which is unexpected here.
    if ((chr & 0xC0) != 0x80) {
      for (int i = 0; i < index; ++i) token[i] = '?';
      state = index = 0;
      return token;
    }

    token[index++] = chr;
    --state;

    if (state == 0) {
      index = 0;
      return token;
    }
  }

  return NULL;
}

static inline bool isHex(wchar_t chr) {
  return (chr >= L'0' && chr <= L'9') || (chr >= L'A' && chr <= L'F') ||
         (chr >= L'a' && chr <= L'f');
}

bool parseHex(HWND hWnd, const wchar_t *hexInput, unsigned char **dest) {
  wchar_t hex[2];
  std::string tmp;
  int hexLen = 0, i;

  for (i = 0; true; ++i) {
    if (isHex(hexInput[i])) {
      if (hexLen >= 2) {
        ui::messageBox(hWnd, L"Each memory value must fit in 8-bit.", L"Error", MB_ICONWARNING);
        *dest = NULL;
        return false;
      }
      if (hexInput[i] > L'Z') {  // Aligns to the upper case.
        hex[hexLen++] = hexInput[i] - (L'a' - L'A');
      } else {
        hex[hexLen++] = hexInput[i];
      }
    } else if (iswspace(hexInput[i]) || !hexInput[i]) {
      if (hexLen == 1) {
        if (hex[0] < L'A') {
          tmp += (char)(hex[0] - L'0');
        } else {
          tmp += (char)(hex[0] - L'A' + 10);
        }
        hexLen = 0;
      } else if (hexLen == 2) {
        if (hex[0] < L'A') {
          tmp += (char)((hex[0] - L'0') << 4);
        } else {
          tmp += (char)((hex[0] - L'A' + 10) << 4);
        }
        if (hex[1] < L'A') {
          tmp[tmp.size() - 1] += hex[1] - L'0';
        } else {
          tmp[tmp.size() - 1] += hex[1] - L'A' + 10;
        }
        hexLen = 0;
      }
    } else {
      ui::messageBox(hWnd, L"Invalid input.", L"Error", MB_ICONWARNING);
      *dest = NULL;
      return false;
    }

    if (hexInput[i] == L'\0') break;
  }

  if (tmp.size() == 0) {
    *dest = NULL;
    return true;
  }

  *dest = (unsigned char *)malloc(tmp.size() + 1);
  if (!*dest) {
    ui::messageBox(hWnd, L"Memory allocation failed.", L"Error", MB_ICONWARNING);
    return false;
  }

  memcpy(*dest, tmp.c_str(), tmp.size() + 1);
  return true;
}

void toHex(unsigned char num, wchar_t *dest) {
  unsigned char high = num >> 4, low = num & 0xF;

  if (high < 10) {
    dest[0] = L'0' + high;
  } else {
    dest[0] = L'A' + (high - 10);
  }

  if (low < 10) {
    dest[1] = L'0' + low;
  } else {
    dest[1] = L'A' + (low - 10);
  }

  dest[2] = L' ';
  dest[3] = 0;
}

enum newline_t convertCRLF(std::wstring &_target, enum newline_t _newLine) {
  std::wstring::iterator iter = _target.begin();
  std::wstring::iterator iterEnd = _target.end();
  std::wstring temp;
  const wchar_t *nl;
  size_t CRs = 0, LFs = 0, CRLFs = 0;
  if (_newLine == NEWLINE_LF) {
    nl = L"\n";
  } else if (_newLine == NEWLINE_CR) {
    nl = L"\r";
  } else {
    nl = L"\r\n";
  }

  if (0 < _target.size()) {
    wchar_t bNextChar = *iter++;

    while (true) {
      if (L'\r' == bNextChar) {
        temp += nl;                  // Newline
        if (iter == iterEnd) break;  // EOF
        bNextChar = *iter++;         // Retrive a character
        if (L'\n' == bNextChar) {
          if (iter == iterEnd) break;  // EOF
          bNextChar = *iter++;         // Retrive a character
          CRLFs++;
        } else {
          CRs++;
        }
      } else if (L'\n' == bNextChar) {
        temp += nl;                    // Newline
        if (iter == iterEnd) break;    // EOF
        bNextChar = *iter++;           // Retrive a character
        if (L'\r' == bNextChar) {      // Broken LFCR, so don't count
          if (iter == iterEnd) break;  // EOF
          bNextChar = *iter++;         // Retrive a character
        } else {
          LFs++;
        }
      } else {
        temp += bNextChar;           // Not a newline
        if (iter == iterEnd) break;  // EOF
        bNextChar = *iter++;         // Retrive a character
      }
    }
  }

  _target = temp;

  if (LFs > CRLFs && LFs >= CRs) {
    return NEWLINE_LF;
  } else if (CRs > LFs && CRs > CRLFs) {
    return NEWLINE_CR;
  } else {
    return NEWLINE_CRLF;
  }
}
}  // namespace util
