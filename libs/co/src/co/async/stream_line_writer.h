#pragma once

#include "co/async/stream.h"
#include "co/async/read_write_all.h"

#include "co/base/buffered_tokenizer.h"

namespace co {
namespace async {

class StreamLineWriter {
public:
  virtual ~StreamLineWriter() = default;
  
  StreamLineWriter(StreamIo& stm_io): stm_io_(stm_io)
  {
  }

  StreamIo& GetStreamIo() { return stm_io_; }

  // |lines_copied| COPIED, free them if you want after calling AsyncWriteLine(s)
  void AsyncWriteLine(const std::string& line, HandlerWithErrcodeSize handler) {
    AsyncWriteLines<std::vector<std::string>>({ line }, handler);
  }

  // |lines_copied| IS COPIED, free them if you want after calling AsyncWriteLine(s)
  template <class PushbackableContainer>
  void AsyncWriteLines(const PushbackableContainer& lines_copied, HandlerWithErrcodeSize handler) {
    Shptr<std::string> serialized_lines(make_shared<std::string>());
    SerializeLines(*serialized_lines.get(), lines_copied);

    // Using AsyncWriteAll -without strand version
    co::async::AsyncWriteAllNostrand(stm_io_,
      boost::asio::const_buffers_1(&serialized_lines->at(0), serialized_lines->length()),
      co::bind(&StreamLineWriter::HandleWriteLines, this, _1, _2, handler, serialized_lines));
  }
  
private:  
  void HandleWriteLines(Errcode err, size_t bytes_written, HandlerWithErrcodeSize user_handler,
    Shptr<std::string> serialized_lines)
  {
    user_handler(err, bytes_written);
  }
  template <class PushbackableContainer>
  void SerializeLines(std::string& serialized_lines, const PushbackableContainer& lines) {
    for (const auto& line : lines) {
      std::string fixed_line(line);
      std::replace(fixed_line.begin(), fixed_line.end(), '\n', ';');
      serialized_lines.append(fixed_line + "\n");
    }
  }
private:
  Shptr<Strand> strand_;
  StreamIo& stm_io_;
};



}}


