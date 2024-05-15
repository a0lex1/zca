#pragma once

#include <exception>

namespace co {

class RecoverableException: public std::exception {
public:
  virtual ~RecoverableException() throw () = default;

  virtual const char* what() const throw () {
    return "recoverable_exception";
  }
};

}


