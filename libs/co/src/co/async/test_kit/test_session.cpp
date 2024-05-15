#include "co/async/test_kit/test_session.h"
#include "co/async/wrap_post.h"

namespace co {
namespace async {
namespace test_kit {


TestSession::TestSession(Uptr<Stream> new_stm,
                         Shptr<Strand> strand,
                         TestSessionParams params,
                         int integer_tag /*= 0*/) :
  Session(std::move(new_stm), strand),
  params_(params),
  integer_tag_(integer_tag)
{
  portions_left_ = params.portion_count;
  if (portions_left_ != 0 && params.portion_size != 0) {
    buf_ = std::string(params.portion_size, params.buffer_fill_char);
  }
}

void TestSession::BeginIo(RefTracker rt)
{
  //syslog(_DBG) << "Starting...\n";
  rt.SetReferencedObject(shared_from_this());
  timer_ = std::make_unique<boost::asio::deadline_timer>(GetStreamIo().GetIoContext());
  DoReadOrWriteAgain(rt);
}

void TestSession::StopUnsafe()
{
  //syslog(_INFO) << "Stopping...\n";
  if (timer_ != nullptr) {
    timer_->cancel(); // throws !!!!!!!!!!!!!!!!!!!!!!
  }
  Session::StopUnsafe();
}

void TestSession::DoReadOrWriteAgain(RefTracker rt)
{
  if (portions_left_ == 0) {
    return;
  }
  if (params_.read_not_write) {
    // No strand because AsyncReadAll calls handler from inside strand
    co::async::AsyncReadAll(
          GetFiberStrandShptr(),
          GetStreamIo(),
          boost::asio::mutable_buffers_1(&buf_[0], buf_.length()),
          co::bind(&TestSession::HandleReadOrWrite, shared_from_this(), _1, _2, rt));
  }
  else {
    co::async::AsyncWriteAll(
          GetFiberStrandShptr(),
          GetStreamIo(),
          boost::asio::const_buffers_1(&buf_[0], buf_.length()),
          co::bind(&TestSession::HandleReadOrWrite, shared_from_this(), _1, _2, rt));
  }
}

void TestSession::HandleReadOrWrite(Errcode err, size_t num_bytes, RefTracker rt)
{
  DCHECK(IsInsideFiberStrand());
  if (err) {
    // Can be boost::asio::error::eof, |num_bytes| shall be 0
    return;
  }
  if (portions_left_ != -1) {
    portions_left_ -= 1;
    if (portions_left_ == 0) {
      Session::StopUnsafe();
      return;
    }
  }
  if (params_.portion_delay.total_nanoseconds() != 0) {
    timer_->expires_from_now(params_.portion_delay);
    timer_->async_wait(
      wrap_post(GetFiberStrand(), co::bind(&TestSession::HandleTimer, shared_from_this(), _1, rt)));
  }
  else {
    DoReadOrWriteAgain(rt);
  }
}

void TestSession::HandleTimer(Errcode err, RefTracker rt)
{
  DCHECK(IsInsideFiberStrand());
  if (err) {
    // No need to save timer's error
    //rt.SetError(err);
    return;
  }
  DoReadOrWriteAgain(rt);
}

}}}


