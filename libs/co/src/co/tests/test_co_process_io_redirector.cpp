#include "co/async/process_io_redirector.h"
#include "co/base/tests.h"

#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace boost::asio;

namespace {
class Drainer {
public:
  Drainer(StreamIo& stm_io): stm_io_(stm_io) {

  }

  void Drain(std::string& user_buffer, HandlerWithErrcode handler) {
    user_buffer_ = &user_buffer;
    user_handler_ = handler;
    ReadCommandAgain();
  }

private:
  void ReadCommandAgain() {
    boost::asio::mutable_buffers_1 buf(buf_, sizeof(buf_));
    stm_io_.AsyncReadSome(buf, co::bind(&Drainer::HandleRead, this, _1, _2));
  }
  void HandleRead(Errcode err, size_t num_bytes) {
    if (err) {
      DCHECK(num_bytes == 0);
      auto hcopy = user_handler_;
      user_handler_ = nullptr;
      hcopy(err);
      return;
    }
    user_buffer_->append(buf_, num_bytes);
    ReadCommandAgain();
  }

private:
  StreamIo& stm_io_;
  string* user_buffer_;
  HandlerWithErrcode user_handler_;
  char buf_[4096];
};
}


void test_co_process_io_redirector(TestInfo& ti) {
  io_context ioc;

  ProcessIoRedirector redirector(ioc);

  const string shell_cmd = "whoami";

  Errcode ec;
  redirector.Exec(shell_cmd, ec);
  DCHECK(!ec);

  string buffer;
  Drainer drainer(redirector);
  drainer.Drain(buffer, [&](Errcode err) {
    syslog(_INFO) << "Drained " << buffer.length() << " bytes:\n";
    syslog(_INFO) << buffer << "\n";
    });

  ioc.run();

  DCHECK(buffer.length() != 0);

}


