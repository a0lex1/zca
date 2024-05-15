#include "netshell/netshell_factory.h"

#include "co/async/wrap_post.h"
#include "co/async/server.h"
#include "co/async/client.h"
#include "co/async/tcp.h"
#include "co/async/test_kit.h"

#include "co/base/strings.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;
using namespace co::async;
using namespace co::async::test_kit;
using co::net::Endpoint;
using co::net::TcpEndpoint;

// Old variant: Loopback network test

static const int kSomeStatus = 150;
static netshell::NsStatusDescriptorTable gTestStatusDescriptors = {
  {kSomeStatus, {"SOME_STATUS", netshell::fCanNotHaveBody}},
};

class ClientSession
  :
  public co::enable_shared_from_this<ClientSession>,
  public Session {
public:
  virtual ~ClientSession() = default;

  using NetshellError = netshell::NetshellError;
  using NsCommandResultReaderText = netshell::NsCommandResultReaderText;
  using NsCommandWriterText = netshell::NsCommandWriterText;
  using NsCmdResult = netshell::NsCmdResult;

  ClientSession(Uptr<Stream> new_stm, Shptr<Strand> strand, size_t num_cmds)
    :
    Session(move(new_stm), strand),
    netshell_rdr_(gTestStatusDescriptors, strand, GetStreamIo(), 4096), //<<<<<<<<<<< max_line_length
    netshell_writ_(strand, GetStreamIo()),
    num_cmds_left_(num_cmds), cur_number_(0)
  {
  }
private:
  virtual void BeginIo(RefTracker rt) override {
    rt.SetReferencedObject(shared_from_this());
    SendCommandAgain(rt);
  }

  void SendCommandAgain(RefTracker rt) {
    if (num_cmds_left_ == 0) {
      //rt.SetError(co::NoError()); // debug: check if it's already set to NoError
      return;
    }
    cur_write_line_ = co::string_printf("%d", cur_number_);
    syslog(_DBG) << "Writing line " << cur_write_line_ << "\n";
    netshell_writ_.AsyncWriteCommand(cur_write_line_,
      wrap_post(GetFiberStrand(), co::bind(&ClientSession::HandleWriteLine, shared_from_this(), _1, rt)));
  }
  void HandleWriteLine(const NetshellError& err, RefTracker rt) {
    if (err) {
      syslog(_DBG) << "err " << err << "\n";
      return;
    }
    netshell_rdr_.AsyncReadResult(cmd_result_,
      wrap_post(GetFiberStrand(), co::bind(&ClientSession::HandleReadNetshellResult, shared_from_this(), _1, rt)));
  }
  void HandleReadNetshellResult(const NetshellError& err, RefTracker rt) {
    using namespace netshell;
    if (err) {
      syslog(_DBG) << "err " << err << "\n";
      return;
    }
    DCHECK(cmd_result_.status_code == kSomeStatus);
    DCHECK(cmd_result_.result_type == NsResultType::kText);
    DCHECK(cmd_result_.text_lines.size() == cur_number_);
    syslog(_DBG) << "Result read: " << cmd_result_.text_lines.size() << " lines\n";
    size_t i = 1;
    size_t char_len = co::string_printf("%d", cur_number_).length();
    for (auto line : cmd_result_.text_lines) {
      DCHECK(line.length() == i*char_len);
      string n(co::string_printf("%d", cur_number_));
      string expected;
      for (size_t j = 0; j < i; j++) {
        expected += n;
      }
      DCHECK(line == expected);
      i += 1;
    }
    cur_number_ += 1;
    num_cmds_left_ -= 1;
    SendCommandAgain(rt);
  }
private:
  NsCommandResultReaderText netshell_rdr_;
  NsCommandWriterText netshell_writ_;
  size_t cur_number_;
  size_t num_cmds_left_;
  string cur_write_line_;
  NsCmdResult cmd_result_;
};

class SmellySession
  :
  public co::enable_shared_from_this<SmellySession>,
  public Session {
public:
  virtual ~SmellySession() = default;

  using NsCmdResult = netshell::NsCmdResult;
  using NsCommandReaderText = netshell::NsCommandReaderText;
  using NsCommandResultWriterText = netshell::NsCommandResultWriterText;

  SmellySession(Uptr<Stream> new_stm, Shptr<Strand> strand)
    :
    Session(move(new_stm), strand),
    netshell_rdr_(strand, GetStreamIo(), 4096),
    netshell_writ_(gTestStatusDescriptors, strand, GetStreamIo())
  {
  }
private:
  virtual void BeginIo(RefTracker rt) override {
    rt.SetReferencedObject(shared_from_this());
    ReadCommandAgain(rt);
  }

  void ReadCommandAgain(RefTracker rt) {
    netshell_rdr_.AsyncReadCommand(cur_read_line_,
      wrap_post(GetFiberStrand(), co::bind(&SmellySession::HandleReadCommand, shared_from_this(), _1, rt)));
  }
  void HandleReadCommand(Errcode err, RefTracker rt) {
    using namespace netshell;
    if (err) {
      syslog(_DBG) << "err " << err << "\n";
      return;
    }
    syslog(_DBG) << "Command read: " << cur_read_line_ << "\n";
    uint32_t number;
    bool ok = co::string_to_uint(cur_read_line_, number, 10);
    DCHECK(ok);
    int32_t RET_CODE = -1;
    result_ = NsCmdResult(kSomeStatus, RET_CODE, NsResultType::kText);
    for (uint32_t i = 0; i < number; i++) {
      string n(co::string_printf("%d", number));
      string generated_line;
      for (size_t j = 0; j < i+1; j++) {
        generated_line += n;
      }
      result_.text_lines.push_back(generated_line);
    }
    syslog(_DBG) << "Writing result " << result_.text_lines.size() << " lines\n";
    netshell_writ_.AsyncWriteResult(result_,
      wrap_post(GetFiberStrand(),
                co::bind(&SmellySession::HandleWriteNetshellResult, shared_from_this(), _1, _2, rt)));
  }
  void HandleWriteNetshellResult(Errcode err, size_t bytes_written, RefTracker rt) {
    if (err) {
      syslog(_DBG) << "err " << err << "\n";
      return;
    }
    result_.text_lines.clear();
    ReadCommandAgain(rt);
  }
private:
  NsCommandReaderText netshell_rdr_;
  NsCommandResultWriterText netshell_writ_;
  string cur_read_line_;
  NsCmdResult result_;
};


void test_netshell(TestInfo&) {
  // Client:
  //  0
  // Server:
  //  result=text;lines=0

  // Client:
  //   1
  // Server:
  //   result=text;lines=1
  //   1

  // Client:
  //   2
  // Server:
  //   result=text;lines=2
  //   2
  //   22

  // Client:
  //   5
  // Server:
  //   result=text;lines=5
  //   5
  //   55
  //   555
  //   5555
  //   55555

  syslog(_TRACE) << "HELLO ! REMOVE ME!\n";

  ThreadModelConfig tmconf;
  ThreadModel tm(tmconf);
  size_t num_cli_cmds = 33;

  io_context& ioc(tm.DefIOC());
  ServerWithSessList server(
    TcpEndpoint("127.0.0.1", 0),
    ServerObjects(
      make_shared<TcpStreamFactory>(ioc),
      make_unique<TcpStreamAcceptor>(ioc),
      ioc),
    [&](Uptr<Stream> s, Shptr<Strand> session_strand) {
      return make_shared<SmellySession>(move(s), session_strand);
    }
  );
  server.PrepareToStartNofail();
  Client client(
    server.GetLocalAddressToConnect(),
    make_shared<TcpStreamConnector>(),
    make_shared<ClientSession>(make_unique<TcpStream>(ioc),
                               make_shared<Strand>(ioc),
                               num_cli_cmds));
  RefTrackerContext rtctx(CUR_LOC());
  bool server_stopped = false, client_stopped = false, all_stopped = false;
  {
    co::RefTracker rt_all(CUR_LOC(), rtctx.GetHandle(),
      [&]() {
        syslog(_INFO) << "All netshell stopped\n";
        all_stopped = true;
      }
    );
    co::RefTracker rt_server(CUR_LOC(),
      [&]() {
        syslog(_INFO) << "netshell server stopped\n";
        server_stopped = true;
      },
      rt_all
    );
    co::RefTracker rt_client(CUR_LOC(),
      [&]() {
        syslog(_INFO) << "netshell client stopped, err\n";
        client_stopped = true;
        server.StopThreadsafe();
      },
      rt_all
    );
    server.Start(rt_server);
    syslog(_INFO) << "netshell server started\n";

    client.PrepareToStartNofail();
    syslog(_INFO) << "Client prepared to start\n";

    client.Start(rt_client);
    syslog(_INFO) << "netshell client started\n";
  }  
  tm.Run();
  syslog(_INFO) << "ThreadModel::Run() returned\n";
  DCHECK(server_stopped);
  DCHECK(client_stopped);
  DCHECK(all_stopped);
}




