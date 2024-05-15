#pragma once

#include <string>

namespace co {

template <typename E, typename I>
class Error {
public:
  virtual ~Error() = default;

  operator bool() const { return static_cast<int>(errc_) != 0; }
  Error(): errc_(static_cast<E>(0)) { }
  Error(E errc, const I& err_info = I()) : errc_(errc), err_info_(err_info) {}
  //Error(E errc, I&& err_info = I()) : errc_(errc), err_info_(std::move(err_info)) {}//remove

  E GetErrc() const { return errc_; }
  const I& GetErrorInfo() const { return err_info_; }

  // override GetErrcTitle() and MakeErrorMessage() for your own errors
  const char* GetErrcTitle() const { return "unknown error"; }
  std::string MakeErrorMessage() const { return GetErrcTitle() + std::string(":"); }

protected:
  const char* DefaultErrcTitleOk() const { return ""; }
  const char* DefaultErrcTitleUnknown() const { return "unknown error"; }
private:
  E errc_;
  I err_info_;
};


}

