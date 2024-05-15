#pragma once

#include <cstdint>
#include <string>

namespace cc {

class BotId {
public:
  BotId();
  
  static const size_t kByteLength = 16;

  const char* GetBytes() const;

  static BotId FromUint(unsigned number); // high part will be 0
  static BotId FromUints(uint64_t hi, uint64_t low);

  std::string ToStringRepr() const;
  bool FromStringRepr(const std::string&);
  
  void Clear();

  bool operator ==(const BotId& r) const;
  bool operator !=(const BotId& r) const;

private:
  BotId(unsigned number);
  BotId(uint64_t hi, uint64_t low);

private:
  union {
    char bytes[16];
    struct {
      uint64_t hi;
      uint64_t lo;
    } i64;
  } data;
};

}

