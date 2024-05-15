#pragma once

#include <string>
#include <ostream>
#include <sstream>

namespace co {

class Textualizable {
public:
  //virtual void Textualize(std::string& append_to) const = 0;

  virtual void Textualize(std::ostream& append_to_stm) = 0;

  void Textualize(std::string& append_to) {
    std::stringstream ss;
    Textualize(ss);
    append_to += ss.str();
  }

  std::string GetTextualized() {
    std::stringstream ss;
    Textualize(ss);
    return ss.str();
  }
};


}


