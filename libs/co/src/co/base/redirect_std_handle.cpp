#include "co/base/redirect_std_handle.h"

#include "co/common.h"

#ifdef _WIN32
#include <Windows.h>
#include <io.h>
#include <fcntl.h>
#include <cstdio>
#include <iostream>
#endif

namespace co {

#ifdef _WIN32
// https://stackoverflow.com/questions/56939802/win32-gui-c-app-redirect-both-stdout-and-stderr-to-the-same-file-on-disk
void RedirectStderrToFile(const char* filename) {

  FILE *stream;
  //if (_wfreopen_s(&stream, filename, L"w", stdout)) __debugbreak();
  //if (freopen_s(&stream, filename, "w", stdout)) __debugbreak();
  if (freopen_s(&stream, filename, "w", stderr)) __debugbreak();
  //if (freopen_s(&stream, "NUL", "w", stderr)) __debugbreak();
  //if (_dup2(_fileno(stdout), _fileno(stderr))) __debugbreak();

  //if (!SetStdHandle(STD_OUTPUT_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(stdout))))) __debugbreak();
  if (!SetStdHandle(STD_ERROR_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(stderr))))) __debugbreak();

/*if (_setmode(_fileno(stdout), _O_WTEXT) == -1) __debugbreak();
  if (_setmode(_fileno(stderr), _O_WTEXT) == -1) __debugbreak();*/

  //if (_setmode(_fileno(stdout), _O_TEXT) == -1) __debugbreak();
  if (_setmode(_fileno(stderr), _O_TEXT) == -1) __debugbreak();
}
#else
void RedirectStderrToFile(const char* file) {
  DCHECK(!"currently only win32 supported");
}
#endif
}


