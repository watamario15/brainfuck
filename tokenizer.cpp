#include "tokenizer.hpp"

#include <string.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

const wchar_t *UTF8Tokenizer::flush() {
  if (index == 0) return NULL;

  memset(result, 0, sizeof(result));
  for (int i = 0; i < index; ++i) result[i] = L'?';

  state = index = 0;

  return result;
}

const wchar_t *UTF8Tokenizer::add(unsigned char chr) {
  if (state == 0) {  // Reading the first character of a token.
    memset(token, 0, sizeof(token));

    // Got a later byte character, which is unexpected here.
    if ((chr & 0xC0) == 0x80) {
      memset(result, 0, sizeof(result));
      result[0] = L'?';
      return result;
    }

    // ASCII
    if (!(chr & 0x80)) {
      memset(result, 0, sizeof(result));
      result[0] = chr;
      return result;
    }

    token[0] = chr;

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
      memset(result, 0, sizeof(result));
      for (int i = 0; i < index; ++i) result[i] = L'?';
      state = index = 0;
      return result;
    }

    token[index++] = chr;
    --state;

    if (state == 0) {
      memset(result, 0, sizeof(result));
      MultiByteToWideChar(CP_UTF8, 0, (char *)token, -1, result,
                          sizeof(result) / sizeof(result[0]));
      index = 0;
      return result;
    }
  }

  return NULL;
}

const wchar_t *SJISTokenizer::flush() {
  // TODO
  return NULL;
}

const wchar_t *SJISTokenizer::add(unsigned char chr) {
  // TODO
  return NULL;
}
