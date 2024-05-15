#pragma once

namespace co {

  // - win32 only yet
  // - unicode: both cout and wcout, all converted to ansi
void RedirectStderrToFile(const char* filename);

}


