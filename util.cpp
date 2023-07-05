#include "util.hpp"

namespace util {
#ifdef UNDER_CE
int messageBox(HWND hWnd, HINSTANCE hInst, const wchar_t *lpText, const wchar_t *lpCaption, unsigned uType) {
  UNREFERENCED_PARAMETER(hInst);
  return MessageBoxW(hWnd, lpText, lpCaption, uType);
}
#else
// The function pointer type for TaskDialog API.
typedef HRESULT(__stdcall *TaskDialog_t)(HWND hwndOwner, HINSTANCE hInstance, const wchar_t *pszWindowTitle,
                                         const wchar_t *pszMainInstruction, const wchar_t *pszContent,
                                         int dwCommonButtons, const wchar_t *pszIcon, int *pnButton);

int messageBox(HWND hWnd, HINSTANCE hInst, const wchar_t *lpText, const wchar_t *lpCaption, unsigned uType) {
  // Tests whether uType uses some features that TaskDialog doesn't support.
  if (uType & ~(MB_ICONMASK | MB_TYPEMASK)) goto mbfallback;

  int buttons;
  switch (uType & MB_TYPEMASK) {
    case MB_OK:
      buttons = 1;
      break;
    case MB_OKCANCEL:
      buttons = 1 + 8;
      break;
    case MB_RETRYCANCEL:
      buttons = 16 + 8;
      break;
    case MB_YESNO:
      buttons = 2 + 4;
      break;
    case MB_YESNOCANCEL:
      buttons = 2 + 4 + 8;
      break;
    default:  // Not supported by TaskDialog.
      goto mbfallback;
  }

  wchar_t *icon;
  switch (uType & MB_ICONMASK) {
    case 0:
      icon = NULL;
      break;
    case MB_ICONWARNING:  // Same value as MB_ICONEXCLAMATION.
      icon = MAKEINTRESOURCEW(-1);
      break;
    case MB_ICONERROR:  // Same value as MB_ICONSTOP and MB_ICONHAND.
      icon = MAKEINTRESOURCEW(-2);
      break;
    default:  // Substitute anything else for the information icon.
      icon = MAKEINTRESOURCEW(-3);
  }

  {
    // Tries to load the TaskDialog API which is a newer substitite of MessageBoxW.
    // This API is per monitor DPI aware but doesn't exist before Windows Vista.
    // To make the system select version 6 comctl32.dll, we don't use the full path here.
    HMODULE comctl32 = LoadLibraryW(L"comctl32.dll");
    if (!comctl32) goto mbfallback;

    TaskDialog_t taskDialog = (TaskDialog_t)(void *)GetProcAddress(comctl32, "TaskDialog");
    if (!taskDialog) {
      FreeLibrary(comctl32);
      goto mbfallback;
    }

    int result;
    taskDialog(hWnd, hInst, lpCaption, L"", lpText, buttons, icon, &result);
    FreeLibrary(comctl32);
    return result;
  }

mbfallback:
  return MessageBoxW(hWnd, lpText, lpCaption, uType);
}
#endif

bool parseHex(HWND hWnd, HINSTANCE hInst, const wchar_t *hexInput, unsigned char **dest, int *length) {
  wchar_t hex[2];
  std::string tmp;
  int hexLen = 0, i;

  for (i = 0; true; ++i) {
    if (isHex(hexInput[i])) {
      if (hexLen >= 2) {
        messageBox(hWnd, hInst, L"Each memory value must fit in 8-bit.", L"Error", MB_ICONWARNING);
        *dest = NULL;
        return false;
      }
      // Aligns to the upper case.
      hex[hexLen++] = hexInput[i] > L'Z' ? hexInput[i] - (L'a' - L'A') : hexInput[i];
    } else if (iswspace(hexInput[i]) || !hexInput[i]) {
      if (hexLen == 1) {
        tmp += hex[0] < L'A' ? (char)(hex[0] - L'0') : (char)(hex[0] - L'A' + 10);
        hexLen = 0;
      } else if (hexLen == 2) {
        tmp += hex[0] < L'A' ? (char)((hex[0] - L'0') << 4) : (char)((hex[0] - L'A' + 10) << 4);
        tmp[tmp.size() - 1] += hex[1] < L'A' ? hex[1] - L'0' : hex[1] - L'A' + 10;
        hexLen = 0;
      }
    } else {
      messageBox(hWnd, hInst, L"Invalid input.", L"Error", MB_ICONWARNING);
      *dest = NULL;
      return false;
    }

    if (hexInput[i] == L'\0') break;
  }

  *length = (int)tmp.size();

  if (*length == 0) {
    *dest = NULL;
    return true;
  }

  *dest = (unsigned char *)malloc(*length);
  if (!*dest) {
    messageBox(hWnd, hInst, L"Memory allocation failed.", L"Internal Error", MB_ICONWARNING);
    *length = 0;
    return false;
  }

  memcpy(*dest, tmp.c_str(), *length);
  return true;
}

void toHex(unsigned char num, wchar_t *dest) {
  unsigned char high = num >> 4, low = num & 0xF;

  dest[0] = high < 10 ? L'0' + high : L'A' + (high - 10);
  dest[1] = low < 10 ? L'0' + low : L'A' + (low - 10);
  dest[2] = L' ';
  dest[3] = 0;
}

enum newline_t convertCRLF(std::wstring &target, enum newline_t newLine) {
  std::wstring::iterator iter = target.begin();
  std::wstring::iterator iterEnd = target.end();
  std::wstring temp;
  size_t CRs = 0, LFs = 0, CRLFs = 0;
  const wchar_t *nl = newLine == NEWLINE_LF ? L"\n" : newLine == NEWLINE_CR ? L"\r" : L"\r\n";

  if (0 < target.size()) {
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

  target = temp;

  return LFs > CRLFs && LFs >= CRs ? NEWLINE_LF : CRs > LFs && CRs > CRLFs ? NEWLINE_CR : NEWLINE_CRLF;
}
}  // namespace util
