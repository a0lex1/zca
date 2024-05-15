#pragma once

#ifdef _WIN32

#include <windows.h>

BOOL APIENTRY MyCreatePipeEx(
  OUT LPHANDLE lpReadPipe,
  OUT LPHANDLE lpWritePipe,
  IN LPSECURITY_ATTRIBUTES lpPipeAttributes,
  IN DWORD nSize,
  DWORD dwReadMode,
  DWORD dwWriteMode);

#endif

