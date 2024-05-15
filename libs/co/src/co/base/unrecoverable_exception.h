#pragma once

namespace co {

class UnrecoverableException: public std::exception {
public:
  virtual ~UnrecoverableException() throw () = default;

  virtual const char* what() const throw () {
    return "unrecoverable_exception";
  }
};

}
