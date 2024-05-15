#include "netshell/netshell_factory.h"
#include "netshell/ns_para_command_executor.h"
#include "netshell/ns_para_command_result_writer_queue_st.h"

#include "co/common_config.h"
#include "co/async/test_kit/default_capsule_object_tester.h"
#include "co/async/loop_object_park.h"
#include "co/async/loop_object_set.h"
#include "co/async/server.h"
#include "co/async/client.h"
#include "co/async/tcp.h"
#include "co/async/wrap_post.h"
#include "co/async/create_for_endpoint.h"
#include "co/async/task_manager.h"
#include "co/base/tests.h"
#include "co/base/strings.h"

#include "co/xlog/xlog.h"

#include <boost/asio/deadline_timer.hpp>

using namespace std;
using namespace co;
using namespace co::async;
using namespace co::async::test_kit;
using namespace netshell;
using namespace co::net;

DEFINE_XLOGGER_SINK("test_ns_para_execer", gNsParaExecerSink);
#define XLOG_CURRENT_SINK gNsParaExecerSink

namespace {

  static const int kResultOk = 111;
  static const int kResultCancelled = 222;
  NsStatusDescriptorTable g_status_descriptors = {
    {kResultOk, {"RESULT_OK", fCanHaveBody}},
    {kResultCancelled, {"RESULT_CANCELLED", fCanHaveBody}}
  };


  class ServerSessionApi {
  public:
    virtual ~ServerSessionApi() = default;
    
  };


  class ServerTask : public TaskImpl, public co::enable_shared_from_this<ServerTask> {
  public:
    virtual ~ServerTask() {
      Log(_TRACE) << "~ServerTask DTOR\n";
    }

    ServerTask(
      io_context& ioc,
      const std::string& cmd,
      Shptr<NsCmdResult> result_shptr,
      Shptr<Strand> strand,
      ServerSessionApi& sessapi)
      :
      TaskImpl(strand),
      cmd_(cmd),
      result_shptr_(result_shptr),
      sessapi_(sessapi),
      timer_(ioc)

    {

    }
  private:
    void BeginIo(RefTracker rt) override {
      StringVector parts;
      string_split(cmd_, " ", parts);
      DCHECK(parts.size() == 2);
      DCHECK(parts[0] == "wait");
      if (!string_to_uint(parts[1], wait_msec_)) {
        NOTREACHED();
      }
      timer_.expires_from_now(boost::posix_time::milliseconds(wait_msec_));
      timer_.async_wait(wrap_post(GetFiberStrand(),
        co::bind(&ServerTask::HandleWait, shared_from_this(), _1, rt)));
    }
    void HandleWait(Errcode err, RefTracker rt) {
      Log(_DBG) << "ServerTask: HandleWait(" << wait_msec_ << "): err=" << err << "\n";
      if (err) {
        *result_shptr_.get() = NsCmdResult(kResultCancelled, 0, NsResultType::kMessage).WithMessageBody("cancelled!!!");
        return;
      }
      *result_shptr_.get() = NsCmdResult(kResultOk, 0, NsResultType::kMessage).WithMessageBody("hi");
    }
    void StopUnsafeExtra() override {
      timer_.cancel();
    }
  private:
    const std::string& cmd_;
    uint32_t wait_msec_;
    Shptr<NsCmdResult> result_shptr_;
    ServerSessionApi& sessapi_;
    boost::asio::deadline_timer timer_;
  };


  class ServerSession
    :
    public Session, public co::enable_shared_from_this<ServerSession>,
    private ServerSessionApi
  {
  public:
    virtual ~ServerSession() {
      Log(_TRACE) << "~ServerSession " << SPTR(this) << " DTOR\n";
      _dbg_dtored_ = true;
    }

    ServerSession(Uptr<Stream> new_stm, Shptr<Strand> strand)
      :
      Session(move(new_stm), strand),
      ns_cmd_rdr_(make_unique<NsCommandReaderText>(
        strand,
        GetStream(),
        co::common_config::kLineReaderMaxLineLen)),
      ns_para_cmdres_writ_(make_unique<NsParaCommandResultWriterText>(
        g_status_descriptors,
        strand,
        GetStream())),
      ns_para_cmdres_writ_qst_(make_unique<NsParaCommandResultWriterQueueST>(
        *ns_para_cmdres_writ_.get())),
      taskmgr_(strand, &GetStream().GetIoContext())
    {

    }

  private:
    void BeginIo(RefTracker rt) override {
      rt.SetReferencedObject(shared_from_this());
      taskmgr_.Start(rt);
      ReadCommandAgain(rt);
    }
    void StopUnsafe() override {
      Log(_DBG) << "ServerSession " << SPTR(this) << ": StopUnsafe\n";
      taskmgr_.StopThreadsafe();
      Session::StopUnsafe();
    }
    void ReadCommandAgain(RefTracker rt) {
      ns_cmd_rdr_->AsyncReadCommand(cur_read_cmd_,
        wrap_post(GetFiberStrand(),
          co::bind(&ServerSession::HandleReadCommand, shared_from_this(), _1, rt)));
    }
    void HandleReadCommand(Errcode err, RefTracker rt) {
      if (err) {
        StopThreadsafe();
        return;
      }
      ExecuteCommand(rt); // branches new task
      ReadCommandAgain(rt);
    }
    void ExecuteCommand(RefTracker rt) {
      Log(_DBG) << "ServerSession: " << SPTR(this) << " ExecuteCommand ~~" << cur_read_cmd_ << "\n";
      auto result_shptr = make_shared<NsCmdResult>();
      Shptr<Strand> task_strand = make_shared<Strand>(GetStream().GetIoContext());
      Shptr<ServerTask> task = make_shared<ServerTask>(
        GetStream().GetIoContext(),
        cur_read_cmd_,
        result_shptr,
        task_strand,
        static_cast<ServerSessionApi&>(*this));

      auto task_handler = wrap_post(GetFiberStrand(),
        co::bind(&ServerSession::HandleExecuteCommand,
          shared_from_this(), cur_cmd_index_, result_shptr, rt));

      ++cur_cmd_index_;

      taskmgr_.ExecuteTask(task, RefTracker(CUR_LOC(), task_handler, rt));
    }

    void HandleExecuteCommand(uint64_t cmd_index, Shptr<NsCmdResult> result_shptr, RefTracker rt) {
      WriteResult(cmd_index, result_shptr, rt);
    }

    void WriteResult(uint64_t cmd_index, Shptr<NsCmdResult> result_shptr, RefTracker rt) {
      Log(_DBG) << "ServerSession " << SPTR(this) << ": Writing result...\n";
      // Write through QueueST
      ns_para_cmdres_writ_qst_->AsyncWriteParallelResult(cmd_index, *result_shptr.get(),
        wrap_post(GetFiberStrand(), co::bind(&ServerSession::HandleWriteResult,
          shared_from_this(), _1, result_shptr, rt)));
    }

    void HandleWriteResult(Errcode err, Shptr<NsCmdResult> result_shptr, RefTracker rt) {
      DCHECK(!_dbg_dtored_);
      Log(_DBG) << "ServerSession " << SPTR(this) << ": HandleWriteResult, err = " << err << "\n";
      // Nothing to do!
      if (err) {
        StopThreadsafe();
        return;
      }
    }

  private:
    TaskManager taskmgr_;
    Uptr<NsCommandReaderText> ns_cmd_rdr_;
    Uptr<NsParaCommandResultWriterText> ns_para_cmdres_writ_; // underlying for:
    Uptr<NsParaCommandResultWriterQueueST> ns_para_cmdres_writ_qst_;
    std::string cur_read_cmd_;
    uint64_t cur_cmd_index_{ 0 };
    bool _dbg_dtored_{ false };
  };


  class ClientSession : public Session, public co::enable_shared_from_this<ClientSession> {
  public:
    virtual ~ClientSession() {
      Log(_TRACE) << "~ClientSession DTOR\n";
    }
    ClientSession(
      size_t total_cmds,
      const std::vector<std::string>& cmds,
      const std::vector<uint32_t>& waits,
      Uptr<Stream> new_stm, Shptr<Strand> strand)
      :
      total_cmds_(total_cmds),
      cmds_(cmds), waits_(waits),
      Session(move(new_stm), strand),
      ns_cmd_writer_(make_unique<NsCommandWriterText>(strand, GetStream())),
      ns_para_rdr_(make_unique<NsParaCommandResultReaderText>(
        g_status_descriptors,
        strand, GetStream())),
      ns_para_executor_(make_unique<NsParaCommandExecutor>(
        *ns_cmd_writer_.get(),
        *ns_para_rdr_.get(),
        strand)),
      timer_(GetStream().GetIoContext())
    {
    }

  private:
    void BeginIo(RefTracker rt) override {
      rt.SetReferencedObject(shared_from_this());

      ScheduleOrExecuteNext(RefTracker(CUR_LOC(),
        []() {
          Log(_DBG) << "ClientSession #ioended\n";
        }, rt));
    }
    void StopUnsafe() override {
      Log(_DBG) << "ClientSession: StopUnsafe\n";
      timer_.cancel();
      Session::StopUnsafe();
    }
    void ScheduleOrExecuteNext(RefTracker rt) {
      if (cmds_sent_++ == total_cmds_) {
        StopThreadsafe();
        return;
      }
      uint32_t wait_msec = NextWait();
      if (wait_msec == 0) {
        Log(_DBG) << "ClientSession: NextWait: executing cmd\n";
        ExecuteCmd(rt);
      }
      else {
        Log(_DBG) << "ClientSession: NextWait: scheduling\n";
        ScheduleWait(wait_msec, rt);
      }
    }
    void ScheduleWait(uint32_t wait_msec, RefTracker rt) {
      timer_.expires_from_now(boost::posix_time::milliseconds(wait_msec));
      timer_.async_wait(
        wrap_post(GetFiberStrand(), co::bind(&ClientSession::HandleWait,
          shared_from_this(), _1, rt)));
    }
    void HandleWait(Errcode err, RefTracker rt) {
      Log(_DBG) << "ClientSession: HandleWait: err=" << err << "\n";
      if (err) {
        StopThreadsafe();
        return;
      }
      ExecuteCmd(rt);
      ScheduleOrExecuteNext(rt);
    }
    void ExecuteCmd(RefTracker rt) {
      auto cmd(NextCmd());
      Log(_DBG) << "ClientSession: Executing cmd:" << cmd << "\n";
      auto result_shptr = make_shared<NsCmdResult>();
      ns_para_executor_->ExecuteCommand(cmd, *result_shptr.get(),
        wrap_post(GetFiberStrand(), co::bind(&ClientSession::HandleExecute,
          shared_from_this(), _1, result_shptr, rt)));
    }
    void HandleExecute(NetshellError ns_err, Shptr<NsCmdResult> result_shptr,
      RefTracker rt)
    {
      Log(_DBG) << "ClientSession: HandleExecute: ns_err=" << ns_err.MakeErrorMessage() << "\n";
      if (ns_err) {
        StopThreadsafe();
        return;
      }
      // Nothing to do. Timer will fire.
    }

    uint32_t NextWait() {
      DCHECK(waits_.size());
      auto& wait(waits_[cur_wait_idx_]);
      ++cur_wait_idx_;
      if (cur_wait_idx_ == waits_.size()) {
        cur_wait_idx_ = 0;
      }
      return wait;
    }

    std::string NextCmd() {
      DCHECK(cmds_.size());
      auto& cmd(cmds_[cur_cmd_idx_]);
      ++cur_cmd_idx_;
      if (cur_cmd_idx_ == cmds_.size()) {
        cur_cmd_idx_ = 0;
      }
      return cmd;
    }

  private:
    size_t cmds_sent_{ 0 };
    size_t total_cmds_;
    const std::vector<std::string>& cmds_;
    const std::vector<uint32_t>& waits_;
    Uptr<NsCommandWriter> ns_cmd_writer_; // underlying
    Uptr<NsParaCommandResultReader> ns_para_rdr_; // underlying
    Uptr<NsParaCommandExecutor> ns_para_executor_;
    boost::asio::deadline_timer timer_;
    uint64_t cur_cmd_idx_{ 0 };
    uint64_t cur_wait_idx_{ 0 };
  };

  // ----------------------------------------------------------------------------------

  class TestObject : public LoopObjectNoreset {
  public:
    virtual ~TestObject() = default;

    TestObject(ThreadModel& tm, size_t num_clients, size_t total_cmds)
      :
      total_cmds_(total_cmds),
      tm_(tm), num_clients_(num_clients)
    {

    }
    void SetCmds(StringVector&& cmds) {
      cmds_ = cmds;
    }
    void SetWaits(std::vector<uint32_t>&& waits) {
      waits_ = waits;
    }

  private:

    void PrepareToStart(Errcode& err) override {
      auto sessfac = [](Uptr<Stream> new_stm, Shptr<Strand> strand) -> Shptr<Session> {
        return make_shared<ServerSession>(move(new_stm), strand);
        };
      srv_ = make_shared<Server>(
        TcpEndpoint("127.0.0.1", 0),
        ServerObjects(
          make_shared<TcpStreamFactory>(tm_.DefIOC()),
          make_unique<TcpStreamAcceptor>(tm_.DefIOC()),
          tm_.DefIOC()),
          sessfac);
      srv_->SetupAcceptorNow(err);
      if (err) {
        return;
      }
      Endpoint loc_addr(srv_->GetLocalAddressToConnect());
      if (false) {
        subset_ = make_shared<LoopObjectSet>(strand_);
        for (size_t i = 0; i < num_clients_; i++) {
          auto new_sess = make_shared<ClientSession>(
            total_cmds_,
            cmds_,
            waits_,
            make_unique<TcpStream>(tm_.DefIOC()),
            make_shared<Strand>(tm_.DefIOC())
          );
          Shptr<LoopObject> cli = make_shared<co::async::Client>(
            loc_addr,
            CreateConnectorForEndpoint(loc_addr),
            new_sess);
          subset_->AddObject(cli);
        }
        subset_->PrepareToStart(err);
      }
      else {
        auto cli_sess = make_shared<ClientSession>(
          total_cmds_, cmds_, waits_,
          make_unique<TcpStream>(tm_.DefIOC()),
          make_shared<Strand>(tm_.DefIOC()));

        cli_ = make_unique<Client>(
          loc_addr,
          CreateConnectorForEndpoint(loc_addr),
          cli_sess);

        cli_->PrepareToStart(err);
      }
      if (err) {
        return;
      }
      srv_->PrepareToStart(err);
      if (err) {
        return;
      }
    }
    void Start(RefTracker rt) override {
      srv_->Start(RefTracker(CUR_LOC(),
        []() {
          Log(_DBG) << "TestObject srv_ IOENDED\n";
        }, rt));
      if (false) {
        subset_->Start(RefTracker(CUR_LOC(), [&]() {
          Log(_DBG) << "TestObject subset_ IOENDED\n";
          srv_->StopThreadsafe();
          }, rt));
      }
      else {
        cli_->Start(RefTracker(CUR_LOC(), [&]() {
          Log(_DBG) << "TestObject cli_ IOENDED\n";
          srv_->StopThreadsafe();
          }, rt));
      }
    }
    void CleanupAbortedStop() override {
      srv_->CleanupAbortedStop();
      if (false) {
        subset_->CleanupAbortedStop();
      }
      else {
        cli_->CleanupAbortedStop();
      }
    }
    void StopThreadsafe() override {
      srv_->StopThreadsafe();
      if (false) {
        subset_->StopThreadsafe();
      }
      else {
        cli_->StopThreadsafe();
      }
    }
  private:
    size_t total_cmds_;
    std::vector<std::string> cmds_;
    std::vector<uint32_t> waits_;
    ThreadModel& tm_;
    Shptr<Strand> strand_;
    size_t num_clients_;
    Shptr<Server> srv_;
    Shptr<LoopObjectSet> subset_;
    Uptr<Client> cli_;
  };

  //class TestObjectV2 // without LoopObjectSet
}

// TODO: uncomment multi Client set

void test_netshell_para_executor(TestInfo& ti) {

  /*{
    DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
      auto obj = make_unique<TestObject>(tm, 10, 2);
      obj->SetCmds({ "wait 33" });
      obj->SetWaits({ 3 });
      return obj;
      });

    tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);
  }*/
  {
    DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
      auto obj = make_unique<TestObject>(tm, 10, 50);
      obj->SetCmds({ "wait 1", "wait 33","wait 121"});
      obj->SetWaits({ 3, 1, 57 });
      return obj;
      });

    tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);
  }
  {
    DefaultCapsuleObjectTester tester(ti, [&](ThreadModel& tm) {
      auto obj = make_unique<TestObject>(tm, 10, 50);
      obj->SetCmds({ "wait 121", "wait 33", "wait 1", "wait 49"});
      obj->SetWaits({ 57, 3, 1, 7 });
      return obj;
      });

    tester.ExecuteExpect(capsule::RunLoop::LoopExitCause::kIterLimitReachedCause);
  }
}
