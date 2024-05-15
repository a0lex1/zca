#include "vlanbleed2/async/test_kit/stm/script_op.h"

// ----------------------------------------------------------------------------------------------------------------------------------

// Script Operation Result methods

SOpRes::SOpRes(Errcode _errcode, size_t _bytecount, const char* _buffer) : errcode(_errcode), bytecount(_bytecount), buffer(_buffer)
{
  ClearBitFlags();
  flags.has_errcode = 1;
  flags.has_bytecount = 1;
  flags.has_buffer = 1;
}

SOpRes::SOpRes(Errcode _errcode, size_t _bytecount) : errcode(_errcode), bytecount(_bytecount)
{
  ClearBitFlags();
  flags.has_errcode = 1;
  flags.has_bytecount = 1;
}

SOpRes::SOpRes(Errcode _errcode) : errcode(_errcode)
{
  ClearBitFlags();
  flags.has_errcode = 1;
}

SOpRes::SOpRes()
{
  ClearBitFlags();
}

void SOpRes::ClearBitFlags()
{
  flags.has_errcode = 0;
  flags.has_bytecount = 0;
  flags.has_buffer = 0;
}

void SOpRes::Textualize(std::ostream& ostm) const
{
  ostm << "[";
  if (flags.has_errcode) {
    ostm << "ERRCODE " << errcode << " ";
  }
  if (flags.has_bytecount) {
    ostm << "BYTECOUNT " << bytecount << " ";
  }
  if (flags.has_buffer) {
    ostm << "BUFFER " << buffer << " ";
  }
  ostm << "]";
}

bool SOpRes::operator!=(const SOpRes& r) const
{
  return !operator==(r);
}

bool SOpRes::operator==(const SOpRes& r) const
{
  if (flags.has_errcode) {
    if (errcode != r.errcode) {
      return false;
    }
  }
  if (flags.has_bytecount) {
    if (bytecount != r.bytecount) {
      return false;
    }
  }
  if (flags.has_buffer) {
    if (buffer != r.buffer) {
      return false;
    }
  }
  return true;
}

// ----------------------------------------------------------------------------------------------------------------------------------

// Script Operation Arguments methods

const std::string& SOpArgs::GetStringArg() const
{
  DCHECK(flags_.has_string_arg);
  return string_arg_;
}

size_t SOpArgs::GetSizeTArg() const
{
  DCHECK(flags_.has_size_t_arg);
  return size_t_arg_;
}

const SOpRes& SOpArgs::GetOpResultArg() const
{
  DCHECK(flags_.has_result_arg);
  return result_arg_;
}

SOpArgs::SOpArgs(const Endpoint& _ep_arg) : endpoint_arg_(_ep_arg)
{
  ClearBitFlags();
  flags_.has_endpoint_arg = 1;
}

SOpArgs::SOpArgs(const SOpRes& _result_arg) : result_arg_(_result_arg)
{
  ClearBitFlags();
  flags_.has_result_arg = 1;
}

SOpArgs::SOpArgs(const std::string& _string_arg, size_t _size_t_arg) : string_arg_(_string_arg), size_t_arg_(_size_t_arg)
{
  ClearBitFlags();
  flags_.has_string_arg = 1;
  flags_.has_size_t_arg = 1;
}

SOpArgs::SOpArgs(size_t _size_t_arg) : size_t_arg_(_size_t_arg)
{
  ClearBitFlags();
  flags_.has_size_t_arg = 1;
}

SOpArgs::SOpArgs(const std::string& _string_arg) : string_arg_(_string_arg)
{
  ClearBitFlags();
  flags_.has_string_arg = 1;
}

SOpArgs::SOpArgs()
{
  ClearBitFlags();
}

void SOpArgs::Textualize(std::ostream& ostm) const
{
  ostm << "[";
  if (flags_.has_string_arg) {
    ostm << "STR " << string_arg_ << " ";
  }
  if (flags_.has_size_t_arg) {
    ostm << "SIZE_T " << size_t_arg_ << " ";
  }
  if (flags_.has_result_arg) {
    ostm << "OP_RES " << result_arg_.GetTextualized() << " ";
  }
  if (flags_.has_endpoint_arg) {
    ostm << "ENDPOINT " << endpoint_arg_.ToString();
  }
  ostm << "]";
}

void SOpArgs::ClearBitFlags()
{
  flags_.has_string_arg = 0;
  flags_.has_size_t_arg = 0;
  flags_.has_result_arg = 0;
  flags_.has_endpoint_arg = 0;
}

SOp::SOp(SOpCode _opcode /*= SOpCode::Op_Invalid*/, const SOpArgs& _arguments /*= SOpArgs()*/) :
  opcode(_opcode), arguments(_arguments)
{

}

// ----------------------------------------------------------------------------------------------------------------------------------

// Script Operation methods

void SOp::Textualize(std::ostream& ostm) const
{
  ostm << "OP{";
  ostm << ScriptOpCodeName(opcode);
  ostm << ", ARGS: ";
  arguments.Textualize(ostm);
  ostm << "}";
}


