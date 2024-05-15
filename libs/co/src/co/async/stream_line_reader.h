#pragma once

#include "co/base/buffered_tokenizer.h"
#include "co/async/stream.h"
#include "co/async/read_write_all.h"

namespace co {
namespace async {

class StreamLineReader {
public:
  virtual ~StreamLineReader() = default;
    
  StreamLineReader(StreamIo& stm_io, size_t max_line_length,
                   bool trim_crs = true)
    : stm_io_(stm_io), tok_buf_(max_line_length, "\n"), trim_crs_(trim_crs)
  {
  }

  StreamIo& GetStreamIo() { return stm_io_; }
  
  void AsyncReadLine(std::string& line, HandlerWithErrcode handler) {
    if (GetNextTokenTrim(line)) {
      PostHandler(handler, NoError());
      return;
    }
    // vector<string> specialization is ignored here because nullptr
    ReadMore<std::vector<std::string>>(&line, nullptr, handler);
  }
  
  template <class PushbackableContainer>
  void AsyncReadLines(PushbackableContainer& lines, HandlerWithErrcode handler) {
    if (GetAllTokensTrim(lines)) {
      for (auto& line : lines) {
        if (line.length()) {
          if (line[line.length() - 1] == '\r') {
            line.resize(line.length() - 1);
          }
        }
      }
      PostHandler(handler, NoError());
      return;
    }
    ReadMore(nullptr, &lines, handler);
  }
  
private:  
  template <class PushbackableContainer>
  void ReadMore(std::string* single_line, PushbackableContainer* many_lines,
    HandlerWithErrcode user_handler)
  {
    // Only one of them must be set
    DCHECK((single_line != nullptr && many_lines == nullptr)
      || (single_line == nullptr && many_lines != nullptr));

    size_t space_len;
    char* space = tok_buf_.GetBufferSpace(space_len);
    if (!space_len) {
      PostHandler(user_handler, GetErrcodeForErrno(ERANGE));
      return;
    }
    stm_io_.AsyncReadSome(boost::asio::mutable_buffers_1(space, space_len),
      co::bind(&StreamLineReader::HandleReadMore<PushbackableContainer>,
        this, _1, _2, single_line, many_lines, user_handler));
  }
  template <class PushbackableContainer>
  void HandleReadMore(Errcode err, size_t bytes_read,
    std::string* single_line,
    PushbackableContainer* many_lines,
    HandlerWithErrcode user_handler)
  {
    // Only one of them must be set
    DCHECK((single_line != nullptr && many_lines == nullptr)
      || (single_line == nullptr && many_lines != nullptr));

    if (err) {
      PostHandler(user_handler, err);
      return;
    }
    tok_buf_.OnBufferSpaceFilled(bytes_read);
    if (single_line != nullptr) {
      // User wants one line
      if (GetNextTokenTrim(*single_line)) {
        PostHandler(user_handler, NoError());
        return;
      }
    }
    else {
      // User wants list of lines
      if (GetAllTokensTrim(*many_lines)) {
        PostHandler(user_handler, NoError());
        return;
      }
    }
    // We need more
    ReadMore(single_line, many_lines, user_handler);
  }
  void PostHandler(HandlerWithErrcode user_handler, Errcode err) {
    boost::asio::post(stm_io_.GetIoContext(),
      [=]() {
        //syslog(_DBG) << "-- handler posted from #" << tag_ << " (from " << _debug_tag_ << ")\n";
        user_handler(err);
      });
  }
  template <class PushbackableContainer>
  void SerializeLines(std::string& serialized_lines, const PushbackableContainer& lines) {
    for (const auto& line : lines) {
      std::string fixed_line(line);
      std::replace(fixed_line.begin(), fixed_line.end(), '\n', ';');
      serialized_lines.append(fixed_line + "\n");
    }
  }
  bool GetNextTokenTrim(std::string& line) {
    if (tok_buf_.GetNextToken(line)) {
      if (trim_crs_) {
        TrimCR(line);
      }
      return true;
    }
    return false;
  }
  template <typename PushbackableContainer>
  bool GetAllTokensTrim(PushbackableContainer& lines) {
    if (tok_buf_.GetAllTokens(lines)) {
      for (auto& line : lines) {
        if (trim_crs_) {
          TrimCR(line);
        }
      }
      return true;
    }
    return false;
  }
  void TrimCR(std::string& subject) {
    if (!subject.length()) {
      return;
    }
    if (subject[subject.length() - 1] == '\r') {
      subject.resize(subject.length() - 1);
    }
  }
private:
  bool trim_crs_;
  StreamIo& stm_io_;
  BufferedTokenizer tok_buf_;
};



}}


