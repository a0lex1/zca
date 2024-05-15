#pragma once

#include <string>
#include <cstdarg>

namespace co {

#ifndef NDEBUG

#define SET_DEBUG_TAG(obj, tag, ...) (obj).__SetDebugTag(tag, ##__VA_ARGS__)
#define GET_DEBUG_TAG(obj) (obj).__GetDebugTag()
#else
#define SET_DEBUG_TAG(obj, tag, ...)
#define GET_DEBUG_TAG(obj) std::string("")
#endif

class DebugTagOwner {
public:
  virtual ~DebugTagOwner() = default;

#ifndef NDEBUG
  // don't call directly, use SET_DEBUG_TAG/GET_DEBUG_TAG
  std::string __GetDebugTag() const {
    return debug_tag_;
  }
  void __SetDebugTagv(const char* fmt, va_list vl) {
    char buf[4096];
    vsnprintf(buf, sizeof(buf), fmt, vl);
    debug_tag_ = buf;
  }
  void __SetDebugTag(const char* fmt, ...) {
    va_list vl;
    va_start(vl, fmt);
    __SetDebugTagv(fmt, vl);
  }
private:
  std::string debug_tag_;
#endif
};

#ifndef NDEBUG
// Real version (debug)
#define MakeDebugTag(s_tag) co::_MakeDebugTagv(s_tag)
#define MakeDebugTagv(tag_fmt, ...) co::_MakeDebugTagv(tag_fmt, __VA_ARGS__)
static DebugTagOwner _MakeDebugTagv(const char* tag_fmt, ...) {
  va_list vl;
  va_start(vl, tag_fmt);
  DebugTagOwner dt;
  dt.__SetDebugTagv(tag_fmt, vl);
  return dt;  
}
#else // Release version (empty fake)
#define MakeDebugTagv(s_tag) co::DebugTagOwner()
#define MakeDebugTag(tag_fmt) co::DebugTagOwner()
#endif

}

