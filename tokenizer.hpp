#ifndef TOKENIZER_HPP_
#define TOKENIZER_HPP_

#include <stddef.h>

class UTF8Tokenizer {
 public:
  UTF8Tokenizer() : state(0), index(0){};

  // Resets and returns the remaining bytes if any, all transformed to "?".
  // Returns NULL if no bytes are left.
  const wchar_t *flush();

  // Adds a byte to the internal buffer until it becomes a character.
  // Returns NULL if more bytes are required, and returns a pointer to a string if a complete
  // character is built or this function gets a invalid UTF-8 byte. Note that this function can
  // return multiple "?" characters if it is given a corrupted UTF-8 sequence.
  const wchar_t *add(unsigned char chr);

 private:
  int state, index;
  unsigned char token[16];
  wchar_t result[16];
};

class SJISTokenizer {
 public:
  SJISTokenizer() : token(0){};

  // Resets and returns the remaining bytes if any, all transformed to "?".
  // Returns NULL if no bytes are left.
  const wchar_t *flush();

  // Adds a byte to the internal buffer until it becomes a character.
  // Returns NULL if more bytes are required, and returns a pointer to a string if a complete
  // character is built or this function gets a invalid Shift_JIS byte. Note that this function can
  // return multiple "?" characters if it is given a corrupted Shift_JIS sequence.
  const wchar_t *add(unsigned char chr);

 private:
  unsigned char token;
  wchar_t result[4];
};

#endif
