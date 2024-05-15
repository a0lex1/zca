#pragma once

#include "co/async/read_write_all.h"
#include "co/async/session.h"
#include "co/xlog/define_logger_sink.h"

#include <boost/asio/deadline_timer.hpp>

namespace co {
namespace async {
namespace test_kit {

// Tags help debugging
class _ClientTag {};
class _ServerTag {};
class _LazerTag {}; // ))

static const size_t kDefaultPortionSize = 256;
static const size_t kDefaultPortionCount = 7;
static const boost::posix_time::time_duration kDefaultPortionDelay = boost::posix_time::milliseconds(50);
static const char kDefaultBufferFillChar = '\xff';

struct TestSessionParams {
  bool read_not_write;
  size_t portion_size;
  size_t portion_count;
  boost::posix_time::time_duration portion_delay;
  char buffer_fill_char;

  TestSessionParams() :
    read_not_write(false),
    portion_size(kDefaultPortionSize),
    portion_count(kDefaultPortionCount),
    portion_delay(kDefaultPortionDelay),
    buffer_fill_char(kDefaultBufferFillChar)
  {
  }

  static TestSessionParams Write(
    size_t _portion_size = kDefaultPortionSize,
    size_t _portion_count = kDefaultPortionCount,
    boost::posix_time::time_duration _portion_delay = kDefaultPortionDelay,
    char _buffer_fill_char = kDefaultBufferFillChar)
  {
    TestSessionParams p;
    p.read_not_write = false;
    p.portion_size = _portion_size;
    p.portion_count = _portion_count;
    p.portion_delay = _portion_delay;
    p.buffer_fill_char = _buffer_fill_char;
    return p;
  }

  static TestSessionParams Read(
    size_t _portion_size = kDefaultPortionSize,
    size_t _portion_count = kDefaultPortionCount,
    boost::posix_time::time_duration _portion_delay = kDefaultPortionDelay,
    char _buffer_fill_char = kDefaultBufferFillChar)
  {
    TestSessionParams p;
    p.read_not_write = true;
    p.portion_size = _portion_size;
    p.portion_count = _portion_count;
    p.portion_delay = _portion_delay;
    p.buffer_fill_char = _buffer_fill_char;
    return p;
  }

  static TestSessionParams WriteAndDisconnect(
    size_t _portion_size = kDefaultPortionSize,
    char _buffer_fill_char = kDefaultBufferFillChar)
  {
    TestSessionParams p;
    p.read_not_write = false;
    p.portion_size = _portion_size;
    p.portion_count = 1;
    //p.portion_delay = 0;
    p.buffer_fill_char = _buffer_fill_char;
    return p;
  }

  static TestSessionParams ConFlood() {
    TestSessionParams p;
    p.read_not_write = false;
    p.portion_size = 0;
    p.portion_count = 0;
    //p.portion_delay = 0;
    p.buffer_fill_char = '\0';
    return p;
  }

  static TestSessionParams ReadForever(
    size_t _portion_size = kDefaultPortionSize,
    boost::posix_time::time_duration _portion_delay = kDefaultPortionDelay,
    char _buffer_fill_char = kDefaultBufferFillChar)
  {
    TestSessionParams p;
    p.read_not_write = true;
    p.portion_size = _portion_size;
    p.portion_count = -1;
    //p.portion_delay = _portion_delay;
    p.buffer_fill_char = _buffer_fill_char;
    return p;
  }

  static TestSessionParams WriteForever(
    size_t _portion_size = kDefaultPortionSize,
    boost::posix_time::time_duration _portion_delay = kDefaultPortionDelay,
    char _buffer_fill_char = kDefaultBufferFillChar)
  {
    TestSessionParams p;
    p.read_not_write = false;
    p.portion_size = _portion_size;
    p.portion_count = -1;
    //p.portion_delay = _portion_delay;
    p.buffer_fill_char = _buffer_fill_char;
    return p;
  }
};

// ---------------------------------------------------

class TestSession;
using TestClientSession = TestSession ;
using TestServerSession = TestSession ;

class TestSession
  :
  public co::enable_shared_from_this<TestSession>,
  public co::async::Session
{
  static const size_t kInfinitePortionSize = 1024 * 16;
  using Stream = co::async::Stream;
  using deadline_timer = boost::asio::deadline_timer;
public:
  virtual ~TestSession() = default;

  TestSession(Uptr<Stream> new_stm,
              Shptr<Strand> strand,
              TestSessionParams params,
              int integer_tag = 0);

protected:
  void BeginIo(RefTracker rt) override;
  void StopUnsafe() override;

  void DoReadOrWriteAgain(RefTracker rt);
  void HandleReadOrWrite(Errcode err, size_t num_bytes, RefTracker rt);
  void HandleTimer(Errcode err, RefTracker rt);
private:
  TestSessionParams params_;
  std::string buf_;
  size_t portions_left_;
  Uptr<deadline_timer> timer_;
  int integer_tag_;
};

}}}



