#include "co/async/process_io_redirector.h"

#ifdef _WIN32
#include "co/base/rand_gen.h"

#include <boost/asio/windows/overlapped_ptr.hpp>

#include <strsafe.h>

using namespace std;

using boost::asio::windows::overlapped_ptr;
using boost::asio::windows::stream_handle;

typedef boost::system::error_code err_code;

namespace co {
namespace async {

ProcessIoRedirector::ProcessIoRedirector(io_context& ioc)
  :
  StreamIo(ioc),
  handleStdinRead_(ioc), handleStdinWrite_(ioc),
  handleStdoutRead_(ioc), handleStdoutWrite_(ioc)
{
  Clear();
}

ProcessIoRedirector::~ProcessIoRedirector() {
  Close();
}

void ProcessIoRedirector::Clear() {
  RtlZeroMemory(&pi_, sizeof(pi_));
}

err_code ProcessIoRedirector::last_error() {
  return err_code(GetLastError(), boost::system::system_category());
}

void ProcessIoRedirector::Close() {
  if (pi_.hProcess) {
    CloseHandle(pi_.hProcess);
    pi_.hProcess = NULL;
  }
  if (pi_.hThread) {
    CloseHandle(pi_.hThread);
    pi_.hThread = NULL;
  }
  if (handleStdinRead_.is_open()) {
    handleStdinRead_.close();
  }
  if (handleStdinWrite_.is_open()) {
    handleStdinWrite_.close();
  }
  if (handleStdoutRead_.is_open()) {
    handleStdoutRead_.close();
  }
  if (handleStdoutWrite_.is_open()) {
    handleStdoutWrite_.close();
  }
  Clear();
}

// this code was in pipeex.c
BOOL ProcessIoRedirector::CreatePipes(
  OUT LPHANDLE lpReadPipe,
  OUT LPHANDLE lpWritePipe,
  IN LPSECURITY_ATTRIBUTES lpPipeAttributes,
  IN DWORD nSize,
  IN DWORD dwReadMode,
  IN DWORD dwWriteMode)
{
  HANDLE ReadPipeHandle, WritePipeHandle;
  DWORD dwError;
  CHAR PipeNameBuffer[MAX_PATH];

  // Only one valid OpenMode flag - FILE_FLAG_OVERLAPPED
  if ((dwReadMode | dwWriteMode) & (~FILE_FLAG_OVERLAPPED)) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  if (nSize == 0) {
    nSize = 4096;
  }

  // Pipe num is global
  static uint64_t gqwPipeSerialNumber = 0;

  // Randomize the pipe name
  uint64_t qwRnd1 = RandGen(GenerateSeed()).RandInt<uint64_t>();
  uint64_t qwRnd2 = RandGen(GenerateSeed()).RandInt<uint64_t>();

  StringCchPrintfA(PipeNameBuffer, MAX_PATH,
    "\\\\.\\Pipe\\%I64u%I64u.%08x.%08x",
    qwRnd1,
    qwRnd2,
    GetCurrentProcessId(),
    gqwPipeSerialNumber++
  );

  ReadPipeHandle = CreateNamedPipeA(
    PipeNameBuffer,
    PIPE_ACCESS_INBOUND | dwReadMode,
    PIPE_TYPE_BYTE | PIPE_WAIT,
    1,             // Number of pipes
    nSize,         // Out buffer size
    nSize,         // In buffer size
    120 * 1000,    // Timeout in ms
    lpPipeAttributes
  );

  if (!ReadPipeHandle) {
    return FALSE;
  }

  WritePipeHandle = CreateFileA(
    PipeNameBuffer,
    GENERIC_WRITE,
    0,                         // No sharing
    lpPipeAttributes,
    OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL | dwWriteMode,
    NULL                       // Template file
  );

  if (INVALID_HANDLE_VALUE == WritePipeHandle) {
    dwError = GetLastError();
    CloseHandle(ReadPipeHandle);
    SetLastError(dwError);
    return FALSE;
  }

  *lpReadPipe = ReadPipeHandle;
  *lpWritePipe = WritePipeHandle;
  return(TRUE);
}

void ProcessIoRedirector::Exec(const string& shell_cmd, err_code& ec) {
  STARTUPINFOA si;
  string cmdline(shell_cmd);

  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = NULL;
  sa.bInheritHandle = TRUE;

  HANDLE hStdinRead, hStdinWrite, hStdoutRead, hStdoutWrite;

  if (CreatePipes(&hStdinRead, &hStdinWrite, &sa, 0, FILE_FLAG_OVERLAPPED,
                                                     FILE_FLAG_OVERLAPPED)) {
    if (SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0)) {
      handleStdinRead_.assign(hStdinRead);
      handleStdinWrite_.assign(hStdinWrite);

      if (CreatePipes(&hStdoutRead, &hStdoutWrite, &sa, 0, FILE_FLAG_OVERLAPPED,
                                                           FILE_FLAG_OVERLAPPED)) {
        if (SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0)) {
          handleStdoutRead_.assign(hStdoutRead);
          handleStdoutWrite_.assign(hStdoutWrite);

          RtlZeroMemory(&si, sizeof(si));
          si.cb = sizeof(si);
          si.dwFlags |= STARTF_USESTDHANDLES;
          si.hStdError = hStdoutWrite;
          si.hStdOutput = hStdoutWrite;
          si.hStdInput = hStdinRead;
          if (
            CreateProcessA(NULL, const_cast<char*>(cmdline.c_str()), NULL,
                           NULL, TRUE, 0, NULL, NULL, &si, &pi_))
          {
            handleStdoutWrite_.close();
            handleStdinRead_.close();
            ec = Errcode();
            return;
          }
        }
      }
    }
  }
  ec = last_error();
  Close();
}

void ProcessIoRedirector::Terminate(int err) {
  if (pi_.hProcess) {
    TerminateProcess(pi_.hProcess, err);
    Close();
  }
}

void ProcessIoRedirector::AsyncReadSome(
  boost::asio::mutable_buffers_1 buf,
  HandlerWithErrcodeSize handler)
{
  overlapped_ptr ovl(GetIoContext(), handler);

  DWORD bytes_read;
  BOOL ok = ReadFile(handleStdoutRead_.native_handle(),
                     buf.data(),
                     (DWORD)buf.size(),
                     &bytes_read,
                     ovl.get()
                     );
  DWORD last_err = GetLastError();
  if (!ok && last_err != ERROR_IO_PENDING) {
    ovl.complete(err_code(last_err, boost::system::system_category()), 0);
  }
  else {
    ovl.release();
  }
}

void ProcessIoRedirector::AsyncWriteSome(
  boost::asio::const_buffers_1 buf, HandlerWithErrcodeSize handler)
{
  overlapped_ptr ovl(GetIoContext(), handler);

  DWORD bytes_written;
  BOOL ok = WriteFile(handleStdinWrite_.native_handle(),
                      buf.data(),
                      (DWORD)buf.size(),
                      &bytes_written,
                      ovl.get()
                      );
  DWORD last_err = GetLastError();
  if (!ok && last_err != ERROR_IO_PENDING) {
    ovl.complete(err_code(last_err, boost::system::system_category()), 0);
  } else {
    ovl.release();
  }
}

}}


#else // POSIX

#include "co/async/detail/argv_parser.hpp"

namespace co {
namespace async {

ProcessIoRedirector::ProcessIoRedirector(io_context& ioc)
  :
  StreamIo(ioc),
  child_pid_(0),
  pstdin_write_(0),
  pstdout_read_(0)
{
}

ProcessIoRedirector::~ProcessIoRedirector() {
  if (pstdin_write_) {
    delete pstdin_write_;
    pstdin_write_ = 0;
  }
  if (pstdout_read_) {
    delete pstdout_read_;
    pstdout_read_ = 0;
  }
}

#define RD 0
#define WR 1

void ProcessIoRedirector::Exec(const std::string& shell_cmd,
                               boost::system::error_code& err) {
  int stdin[2], stdout[2];

  pipe(stdin);
  pipe(stdout);

  child_pid_ = fork();

  if (child_pid_ == -1) {
    // Fork: Failed --------------------------------------
    err = boost::asio::error::access_denied; // TODO
    return;
  }
  if (child_pid_) {
    // Fork: Parent --------------------------------------
    close(stdin[RD]);
    close(stdout[WR]);

    pstdin_write_ = new stream_descriptor(GetIoContext(), stdin[WR]);
    pstdout_read_ = new stream_descriptor(GetIoContext(), stdout[RD]);
  }
  else {
    // Fork: Child --------------------------------------
    dup2(stdin[RD], STDIN_FILENO);
    dup2(stdout[WR], STDOUT_FILENO);

    close(stdin[RD]);
    close(stdin[WR]);
    close(stdout[RD]);
    close(stdout[WR]);

//    char cmd_copy[1024];
//    strncpy(cmd_copy, shell_cmd.c_str(), sizeof(cmd_copy)-1);
//    cmd_copy[shell_cmd.length()] = '\0';

    argv_parser<char> ap;
    ap.parse(shell_cmd.c_str());

    execvp(ap.argv()[0], ap.argv());

    abort();
  }
}

void ProcessIoRedirector::Terminate(int err) {
  kill(child_pid_, SIGTERM);

  pstdin_write_->close();
  pstdout_read_->close();

  delete pstdin_write_;
  pstdin_write_ = 0;

  delete pstdout_read_;
  pstdout_read_ = 0;
}

void ProcessIoRedirector::AsyncReadSome(boost::asio::mutable_buffers_1 buf, HandlerWithErrcodeSize handler) {
  if (!pstdout_read_) {
    handler(boost::asio::error::bad_descriptor, 0);
    return;
  }
  pstdout_read_->async_read_some(buf, handler);
}

void ProcessIoRedirector::AsyncWriteSome(boost::asio::const_buffers_1 buf, HandlerWithErrcodeSize handler) {
  if (!pstdin_write_) {
    handler(boost::asio::error::bad_descriptor, 0);
    return;
  }
  pstdin_write_->async_write_some(buf, handler);
}

}}

#endif

