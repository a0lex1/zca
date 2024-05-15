#pragma once

#include "co/async/stream.h"

#ifdef _WIN32
#include <boost/asio/windows/stream_handle.hpp>
#include <windows.h>
#else // POSIX
#include <boost/asio/posix/stream_descriptor.hpp>
#endif

#include <string>

namespace co {
namespace async {

class ProcessIoRedirector : public StreamIo {
public:
  virtual ~ProcessIoRedirector();

  ProcessIoRedirector(boost::asio::io_context& ioc);

  void Exec(const std::string& shell_cmd, boost::system::error_code&);

  void Terminate(int err);

  void AsyncReadSome(boost::asio::mutable_buffers_1 buf, HandlerWithErrcodeSize handler) override;
  void AsyncWriteSome(boost::asio::const_buffers_1 buf, HandlerWithErrcodeSize handler) override;

private:
#ifdef _WIN32
  BOOL CreatePipes(OUT LPHANDLE, OUT LPHANDLE, IN LPSECURITY_ATTRIBUTES, IN DWORD, IN DWORD, IN DWORD);
  static boost::system::error_code last_error();
  static boost::system::error_code no_error();

  void Clear();
  void Close();

  PROCESS_INFORMATION pi_;
  boost::asio::windows::stream_handle handleStdinRead_;
  boost::asio::windows::stream_handle handleStdinWrite_;
  boost::asio::windows::stream_handle handleStdoutRead_;
  boost::asio::windows::stream_handle handleStdoutWrite_;

#else // POSIX

  typedef boost::asio::posix::stream_descriptor stream_descriptor;

  int child_pid_;
  stream_descriptor* pstdin_write_;
  stream_descriptor* pstdout_read_;
#endif
};

}}



