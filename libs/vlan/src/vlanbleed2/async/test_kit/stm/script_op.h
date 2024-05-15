#pragma once

#include "vlanbleed2/async/test_kit/stm/script_opcode.h"

#include "co/net/endpoint.h"
#include "co/base/textualizable.h"
#include "co/common.h"

class SOp;     // Operation
class SOpArgs; // Operation Arguments
class SOpRes;  // Operation Result

// Script Operation Result
struct SOpRes : public co::Textualizable {
  virtual ~SOpRes() = default;

  // Fields

  // Bit flags
  struct {
    int has_errcode : 1;
    int has_bytecount : 2;
    int has_buffer : 3;
  } flags;

  Errcode errcode;
  size_t bytecount{ 0 };
  std::string buffer;

  // Methods

  SOpRes();

  SOpRes(Errcode _errcode);

  SOpRes(Errcode _errcode, size_t _bytecount);

  SOpRes(Errcode _errcode, size_t _bytecount, const char* _buffer);

  // [Textualizable impl]
  void Textualize(std::ostream& ostm) const override;

  bool operator==(const SOpRes& r) const;
  bool operator!=(const SOpRes& r) const;

private:
  void ClearBitFlags();
};

// Script Operation Arguments
class SOpArgs : public co::Textualizable {
public:
  virtual ~SOpArgs() = default;

  using Endpoint = co::net::Endpoint;

  const std::string& GetStringArg() const;
  size_t GetSizeTArg() const;
  const SOpRes& GetOpResultArg() const;

  SOpArgs();
  SOpArgs(const std::string& _string_arg);
  SOpArgs(size_t _size_t_arg);
  SOpArgs(const std::string& _string_arg, size_t _size_t_arg);
  SOpArgs(const SOpRes& _result_arg);
  SOpArgs(const Endpoint& _ep_arg);

  void Textualize(std::ostream& ostm) const override;

private:
  void ClearBitFlags();

private:
  // Bit flags
  struct {
    int has_string_arg : 1;
    int has_size_t_arg : 2;
    int has_result_arg : 3;
    int has_endpoint_arg : 4;
  } flags_;

  // Args
  std::string string_arg_;
  size_t size_t_arg_{ 0 };

  // for XxxEQ/LEQ/GEQ/
  SOpRes result_arg_;

  // for something like Connect, Accept
  Endpoint endpoint_arg_;
};

// Script Operation
struct SOp : public co::Textualizable {
  virtual ~SOp() = default;

  // Fields

  SOpCode opcode;
  SOpArgs arguments;

  // Methods

  SOp(SOpCode _opcode = SOpCode::Op_Invalid,
      const SOpArgs& _arguments = SOpArgs());

  void Textualize(std::ostream& ostm) const override;
};

