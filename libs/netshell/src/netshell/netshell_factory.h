#pragma once

#include "netshell/netshell_default_config.h"

#include "netshell/detail/string_reader.h"
#include "netshell/textualizer.h"
#include "netshell/untextualizer.h"

#include "co/async/stream.h"
#include "co/async/cleanupable.h"
#include "co/async/fibered.h"
#include "co/async/stream_line_reader.h"
#include "co/async/stream_line_writer.h"

namespace netshell {

using HandlerWithNetshellErr = Func<void(NetshellError)>;


// base for async and sync NsCommandReaderBase
class NsCommandReaderBase : public co::async::Cleanupable {
public:
  virtual ~NsCommandReaderBase() = default;
};

class NsCommandReader : public NsCommandReaderBase {
public:
  virtual ~NsCommandReader() = default;

  virtual void AsyncReadCommand(std::string& cmd, HandlerWithErrcode handler) = 0;
  virtual void AsyncReadCommands(StringVector& cmd_list, HandlerWithErrcode handler) = 0;
  // Cleanupable::CleanupAbortedStop
};


// base for async and sync NsCommandWriter
class NsCommandWriterBase : public co::async::Cleanupable {
public:
  virtual ~NsCommandWriterBase() = default;
};

class NsCommandWriter : public NsCommandWriterBase {
public:
  virtual ~NsCommandWriter() = default;

  virtual void AsyncWriteCommand(const std::string& cmd,
    HandlerWithNetshellErr handler) = 0;
  // Cleanupable::CleanupAbortedStop
};


// base for async and sync NsCommandResultReader
class NsCommandResultReaderBase : public co::async::Cleanupable {
public:
  virtual ~NsCommandResultReaderBase() = default;
};

class NsCommandResultReader : public NsCommandResultReaderBase {
public:
  virtual ~NsCommandResultReader() = default;

  virtual void AsyncReadResult(NsCmdResult& cmd_result,
    HandlerWithNetshellErr handler) = 0;
  // Cleanupable::CleanupAbortedStop
};


// base for async and sync NsParaCommandResultReader
class NsParaCommandResultReaderBase : public co::async::Cleanupable {
public:
  virtual ~NsParaCommandResultReaderBase() = default;
};

class NsParaCommandResultReader : public NsParaCommandResultReaderBase {
public:
  virtual ~NsParaCommandResultReader() = default;

  virtual void AsyncReadParallelResult(uint64_t& cmd_index, NsCmdResult& cmd_result,
    HandlerWithNetshellErr handler) = 0;
  // Cleanupable::CleanupAbortedStop
};


// base for async and sync NsCommandResultWriter
class NsCommandResultWriterBase : public co::async::Cleanupable {
public:
  virtual ~NsCommandResultWriterBase() = default;
};

class NsCommandResultWriter : public NsCommandResultWriterBase {
public:
  virtual ~NsCommandResultWriter() = default;

  virtual void AsyncWriteResult(const NsCmdResult& result,
    HandlerWithErrcodeSize handler) = 0;
  // Cleanupable::CleanupAbortedStop
};


// base for async and sync NsParaCommandResultWriter
class NsParaCommandResultWriterBase : public co::async::Cleanupable {
public:
  virtual ~NsParaCommandResultWriterBase() = default;
};

class NsParaCommandResultWriter : public NsParaCommandResultWriterBase {
public:
  virtual ~NsParaCommandResultWriter() = default;

  virtual void AsyncWriteParallelResult(uint64_t cmd_index,
    const NsCmdResult& cmd_result, HandlerWithErrcodeSize handler) = 0;

  virtual Shptr<Strand> GetStrand() = 0;
};

// ---

class NsClientFactory {
public:
  virtual ~NsClientFactory() = default;

  using StreamIo = co::async::StreamIo;

  virtual Uptr<NsCommandWriter> CreateCommandWriter(
    Shptr<Strand> strand,
    StreamIo&) = 0;

  virtual Uptr<NsCommandResultReader> CreateCommandResultReader(
    const NsStatusDescriptorTable& status_descriptors,
    Shptr<Strand> strand,
    StreamIo&,
    size_t max_line_len = kNsMaxLineLen) = 0;

  virtual Uptr<NsParaCommandResultReader> CreateParallelCommandResultReader(
    const NsStatusDescriptorTable& status_descriptors,
    Shptr<Strand> strand,
    StreamIo&,
    size_t max_line_len = kNsMaxLineLen) = 0;
};

class NsServerFactory {
public:
  virtual ~NsServerFactory() = default;

  using StreamIo = co::async::StreamIo;

  virtual Uptr<NsCommandReader> CreateCommandReader(
    Shptr<Strand> strand,
    StreamIo&,
    size_t max_line_len = kNsMaxLineLen) = 0;

  virtual Uptr<NsCommandResultWriter> CreateCommandResultWriter(
    const NsStatusDescriptorTable& status_descriptors,
    Shptr<Strand> strand,
    StreamIo&) = 0;

  virtual Uptr<NsParaCommandResultWriter> CreateParallelCommandResultWriter(
    const NsStatusDescriptorTable& status_descriptors,
    Shptr<Strand> strand,
    StreamIo&) = 0;
};

// --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

// Implementations

class NsCommandReaderText : public NsCommandReader, public co::async::Fibered {
public:
  virtual ~NsCommandReaderText() = default;
  NsCommandReaderText(Shptr<Strand> strand,
                      co::async::StreamIo& stm_io,
                      size_t max_line_len)
    : Fibered(strand), line_rdr_(stm_io, max_line_len)
  {
  }

  void AsyncReadCommand(std::string& cmd, HandlerWithErrcode handler) override;
  void AsyncReadCommands(StringVector& cmd_list, HandlerWithErrcode handler) override;

  void CleanupAbortedStop() override {

  }

private:
  co::async::StreamLineReader line_rdr_;
};

class NsCommandWriterText : public NsCommandWriter, public co::async::Fibered {
public:
  virtual ~NsCommandWriterText() = default;

  NsCommandWriterText(Shptr<Strand> strand, co::async::StreamIo& stm_io)
    : Fibered(strand), line_writ_(stm_io)
  {
  }

  void AsyncWriteCommand(const std::string& cmd, HandlerWithNetshellErr handler) override;
  void CleanupAbortedStop() override {

  }

private:
  co::async::StreamLineWriter line_writ_;
};

class NsCommandResultReaderText : public NsCommandResultReader, public co::async::Fibered {
public:
  virtual ~NsCommandResultReaderText() = default;

  NsCommandResultReaderText(
    const NsStatusDescriptorTable& status_descriptors,
    Shptr<Strand> strand,
    co::async::StreamIo& stm_io,
    size_t max_line_len = kNsMaxLineLen)
    :
    Fibered(strand), status_descriptors_(status_descriptors),
    line_rdr_(stm_io, max_line_len), user_read_result_(nullptr)
  {
  }
  void AsyncReadResult(NsCmdResult& cmd_result, HandlerWithNetshellErr handler) override;

  void CleanupAbortedStop() override;

private:
  void HandleReadFirstLine(Errcode);
  void ReadBodyLines();
  void HandleReadBodyLine(Errcode);
  void PostAndClearReadHandler(const NetshellError&);

private:
  friend class NsParaCommandResultReaderText; // we need to share the line reader
  co::async::StreamLineReader& GetLineReader() { return line_rdr_; }

private:
  const NsStatusDescriptorTable& status_descriptors_;
  Uptr<NsCmdResultUntextualizer> untexer_;
  HandlerWithNetshellErr user_read_handler_;
  NsCmdResult* user_read_result_;
  co::async::StreamLineReader line_rdr_;
  std::string first_line_; // don't forget to clear
  //NsCmdResult ns_result_;
  size_t lines_left_{ 0 };
  std::string more_line_;
};

// NsParaCommandResultReaderText is single fibered, don't call AsyncReadParallelResult
// in parallel.
// We're aggregating NsCommandResultReader, so we're not Fibered, we're recycling
// |ns_rdr_text_|'s strand.
class NsParaCommandResultReaderText : public NsParaCommandResultReader {
public:
  virtual ~NsParaCommandResultReaderText() = default;

  NsParaCommandResultReaderText(
    const NsStatusDescriptorTable& status_descriptors,
    Shptr<Strand> strand,
    co::async::StreamIo& stm_io,
    size_t max_line_len = kNsMaxLineLen)
    :
    ns_rdr_text_(status_descriptors, strand, stm_io, max_line_len)
  {
  }

  void AsyncReadParallelResult(uint64_t& cmd_index, NsCmdResult& cmd_result,
    HandlerWithNetshellErr handler) override;

  void CleanupAbortedStop() override;

private:
  void HandleReadCmdindexLine(Errcode);
  void PostAndClearReadHandler(const NetshellError&);

private:
  uint64_t* user_cmd_index_{ nullptr };
  NsCmdResult* user_cmd_result_{ nullptr }; // same field in NsCommandResultReaderText
  HandlerWithNetshellErr user_handler_;
  std::string cmdindex_line_;
  NsCommandResultReaderText ns_rdr_text_;
  ThreadIdType entered_from_tid_{0};
};


class NsCommandResultWriterText : public NsCommandResultWriter, public co::async::Fibered {
public:
  virtual ~NsCommandResultWriterText() = default;

  NsCommandResultWriterText(const NsStatusDescriptorTable& status_descriptors,
    Shptr<Strand> strand, co::async::StreamIo& stm_io)
    :
    Fibered(strand),
    status_descriptors_(status_descriptors),
    line_writ_(stm_io)
  {
  }

  void AsyncWriteResult(const NsCmdResult& result,
    HandlerWithErrcodeSize handler) override;

  void CleanupAbortedStop() override {}
private:
  const NsStatusDescriptorTable& status_descriptors_;
  co::async::StreamLineWriter line_writ_;
};


class NsParaCommandResultWriterText : public NsParaCommandResultWriter, public co::async::Fibered {
public:
  virtual ~NsParaCommandResultWriterText() = default;

  NsParaCommandResultWriterText(const NsStatusDescriptorTable& status_descriptors,
    Shptr<Strand> strand, co::async::StreamIo& stm_io);

  void AsyncWriteParallelResult(uint64_t cmd_index,
    const NsCmdResult& cmd_result, HandlerWithErrcodeSize handler) override;

  Shptr<Strand> GetStrand() override { return GetFiberStrandShptr(); }

private:
  void HandleWriteCmdindexLine(Errcode err, size_t, const NsCmdResult& cmd_result,
    HandlerWithErrcodeSize user_handler);

  void HandleWriteResult(Errcode err, size_t, size_t, HandlerWithErrcodeSize);

  void CleanupAbortedStop() override;

private:
  co::async::StreamLineWriter line_writ_;
  NsCommandResultWriterText underlying_;
};

// ---

class NsClientFactoryText : public NsClientFactory {
public:
  virtual ~NsClientFactoryText() = default;

  using StreamIo = co::async::StreamIo;

  Uptr<NsCommandWriter> CreateCommandWriter(
    Shptr<Strand> strand,
    StreamIo& stm_io) override
  {
    return make_unique<NsCommandWriterText>(strand, stm_io);
  }

  Uptr<NsCommandResultReader> CreateCommandResultReader(
    const NsStatusDescriptorTable& status_descriptors,
    Shptr<Strand> strand,
    StreamIo& stm_io,
    size_t max_line_len = kNsMaxLineLen) override
  {
    return make_unique<NsCommandResultReaderText>(
      status_descriptors,
      strand, stm_io, max_line_len);
  }

  Uptr<NsParaCommandResultReader> CreateParallelCommandResultReader(
    const NsStatusDescriptorTable& status_descriptors,
    Shptr<Strand> strand,
    StreamIo& stm_io,
    size_t max_line_len = kNsMaxLineLen) override
  {
    return make_unique<NsParaCommandResultReaderText>(
      status_descriptors,
      strand, stm_io, max_line_len);
  }
};

class NsServerFactoryText : public NsServerFactory {
public:
  virtual ~NsServerFactoryText() = default;

  using StreamIo = co::async::StreamIo;

  Uptr<NsCommandResultWriter> CreateCommandResultWriter(
    const NsStatusDescriptorTable& status_descriptors,
    Shptr<Strand> strand, StreamIo& stm_io) override
  {
    return make_unique<NsCommandResultWriterText>(
      status_descriptors,
      strand, stm_io);
  }
  Uptr<NsCommandReader> CreateCommandReader(
    Shptr<Strand> strand,
    StreamIo& stm_io,
    size_t max_line_len = kNsMaxLineLen) override
  {
    return make_unique<NsCommandReaderText>(strand, stm_io, max_line_len);
  }

  Uptr<NsParaCommandResultWriter> CreateParallelCommandResultWriter(
    const NsStatusDescriptorTable& status_descriptors,
    Shptr<Strand> strand,
    StreamIo& stm_io) override
  {
    return make_unique<NsParaCommandResultWriterText>(status_descriptors, strand, stm_io);

  }
private:
  size_t max_line_len_;
};

}


