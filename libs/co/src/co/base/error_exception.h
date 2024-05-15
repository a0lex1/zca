#pragma once

#include "co/base/error.h"
#include <exception>

namespace co {

template <typename Err>
class ErrorException: public std::exception {
public:
  virtual ~ErrorException() throw () = default;

  using ErrorType =  Err;

  ErrorException(const ErrorType& error) : error_(error) {}
  ErrorException(ErrorType&& error) : error_(std::move(error)) {}

  virtual const char* what() const throw () {
    if (msg_.empty()) {
      msg_ = error_.MakeErrorMessage();
    }
    return msg_.c_str();
  }
  const ErrorType& GetError() const { return error_; }
private:
  ErrorType error_;
  mutable std::string msg_;
};

}

