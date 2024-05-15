#pragma once

#include "co/async/stream_chunk_reader.h"
#include "co/base/error.h"
#include <sstream>

enum class ProtoErrc {
  ok = 0,
  stream_chunk_reader_error = 10,
  unserialize_failed = 30
};

class ProtoErrorInfo {
public:
  using StreamChunkReaderError = co::async::StreamChunkReaderError;

  ProtoErrorInfo(const StreamChunkReaderError& scr_err = StreamChunkReaderError())
    : scr_err_(scr_err)
  {
  }
  const StreamChunkReaderError& GetStreamChunkReaderError() const { return scr_err_; }

private:
  StreamChunkReaderError scr_err_;
};

class ProtoError : public co::Error<ProtoErrc, ProtoErrorInfo> {
public:
  virtual ~ProtoError() = default;

  using Error::Error;

  static ProtoError NoError() { return ProtoError(); }

  bool IsStreamEOF() const {
    // the only field that carries network error (`stream error`) is this:
    auto stm_err = GetErrorInfo().GetStreamChunkReaderError().GetErrorInfo().GetStreamError();
    return stm_err == boost::asio::error::eof;
  }
  const char* GetErrcTitle() const {
    switch (GetErrc()) {
    case ProtoErrc::ok: return DefaultErrcTitleOk();
    case ProtoErrc::stream_chunk_reader_error: return "stream chunk reader error";
    case ProtoErrc::unserialize_failed: return "unserialize failed";
    default: return DefaultErrcTitleUnknown();
    }
  }
  std::string MakeErrorMessage() const {
    switch (GetErrc()) {
    case ProtoErrc::ok: return DefaultErrcTitleOk();
    case ProtoErrc::stream_chunk_reader_error: return GetErrcTitle() + std::string(" - ") + GetErrorInfo().GetStreamChunkReaderError().MakeErrorMessage();
    case ProtoErrc::unserialize_failed: return GetErrcTitle();
    default: return DefaultErrcTitleUnknown();
    }
  }
};



