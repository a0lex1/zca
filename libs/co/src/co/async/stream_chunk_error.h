#pragma once

#include "co/base/error.h"
#include "co/base/strings.h"
#include <sstream>

namespace co {
namespace async {

enum class StreamChunkReaderErrc {
  ok = 0,
  stream_error = 911,
  body_size_limit_violated = 277,
  unexpected_eof_header = 666,
  unexpected_eof_body = 337
};

class StreamErrorContainer {
public:
  StreamErrorContainer(Errcode stream_err = Errcode()): stream_err_(stream_err)
  {
  }
  Errcode GetStreamError() const {
    return stream_err_;
  }
  std::string StreamErrorToString() const {
    std::stringstream ss;
    ss << stream_err_;
    return ss.str();
  }
private:
  Errcode stream_err_;
};

// TODO: Other errorinfos->containers
class StreamChunkReaderErrorInfo : public StreamErrorContainer {
public:
  // honestly i dont know how it works
  explicit StreamChunkReaderErrorInfo(Errcode stream_err = Errcode())
    : StreamErrorContainer(stream_err)
  {
  }
  explicit StreamChunkReaderErrorInfo(size_t size_val)
    : size_val_(size_val)
  {
  }
  //GetStreamError()
  //StreamErrorToString()
  size_t GetSizeVal() const { return size_val_; }
private:
  size_t size_val_;
};
  
class StreamChunkReaderError : public co::Error<StreamChunkReaderErrc, StreamChunkReaderErrorInfo> {
public:
  virtual ~StreamChunkReaderError() = default;

  using Error::Error;

  static StreamChunkReaderError NoError() { return StreamChunkReaderError(); }

  bool IsStreamEOF() const {
    return GetErrorInfo().GetStreamError() == boost::asio::error::eof;
  }

  const char* GetErrcTitle() const {
    switch (GetErrc()) {
    case StreamChunkReaderErrc::ok: return DefaultErrcTitleOk();
    case StreamChunkReaderErrc::stream_error: return "stream error";
    case StreamChunkReaderErrc::body_size_limit_violated: return "body size limit violated";
    case StreamChunkReaderErrc::unexpected_eof_header: return "unexpected eof in header";
    case StreamChunkReaderErrc::unexpected_eof_body: return "unexpected eof in body";
    default: return DefaultErrcTitleUnknown();
    }
  }
  std::string MakeErrorMessage() const {
    switch (GetErrc()) {
    case StreamChunkReaderErrc::ok: return DefaultErrcTitleOk();
    case StreamChunkReaderErrc::stream_error:
      return GetErrcTitle()
        + std::string(" - ")
        + GetErrorInfo().StreamErrorToString();
    case StreamChunkReaderErrc::body_size_limit_violated: {
      std::stringstream ss;
      size_t sizeval;
      sizeval = GetErrorInfo().GetSizeVal();
      ss << "body size limit violated, got length " << sizeval << " (0x" << std::hex << sizeval << ")";
      return ss.str();
    }
    case StreamChunkReaderErrc::unexpected_eof_header: return GetErrcTitle() + std::string(", ") + "yyy";
    case StreamChunkReaderErrc::unexpected_eof_body: return GetErrcTitle() + std::string(", ") + "yyy";
    default:
      return DefaultErrcTitleUnknown();
    }
  }
};

}}
