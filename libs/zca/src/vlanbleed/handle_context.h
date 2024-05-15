#pragma once

#include "./vlan_kernel_config.h"
#include "./vlan_common.h"

#include <boost/asio/buffer.hpp>
#include <boost/asio/streambuf.hpp>

// Doesn't control ????????????????????????????????
class HandleContext {
public:
  using const_buffers_1 = boost::asio::const_buffers_1;
  using mutable_buffers_1 = boost::asio::mutable_buffers_1;

  HandleContext(const VlanKernelConfig& kconfig)
    :
    input_stmbuf_(kconfig.buffer_size)
  {
    //input_stmbuf_.prepare();
  }

  bool CommitReadData(const void* buf, size_t buflen) {
    if (input_stmbuf_.size() < buflen) {
      return false;
    }
    memcpy(const_cast<void*>(input_stmbuf_.data().data()),
           buf,
           buflen);
    input_stmbuf_.commit(buflen);
    return true;
  }
  bool CommitReadData(const_buffers_1 from_buf) {
    return CommitReadData(from_buf.data(), from_buf.size());
  }
  void ConsumeReadData(mutable_buffers_1 to_buf, size_t& consumed) {
    size_t num_copy;
    if (to_buf.size() > input_stmbuf_.in_avail()) {
      num_copy = input_stmbuf_.size();
    }
    else {
      num_copy = to_buf.size();
    }
    if (num_copy == 0) {
      consumed = 0;
      return;
    }
    memcpy(to_buf.data(), input_stmbuf_.data().data(), num_copy);
    input_stmbuf_.consume(num_copy);
    consumed = num_copy;
  }
  size_t NumAvailableForRead()  { return input_stmbuf_.in_avail(); }

  bool IsUserReadSet() const { return cur_user_read_handler_ != nullptr; }
  mutable_buffers_1 GetUserReadBuffer() {
    DCHECK(IsUserReadSet());
    return *cur_user_read_buf_.get();
  }
  HandlerWithVlErrSize GetUserReadHandler() {
    DCHECK(IsUserReadSet());
    return cur_user_read_handler_;
  }
  void SetUserRead(mutable_buffers_1 user_buf,
                   HandlerWithVlErrSize user_handler) {
    cur_user_read_buf_ = std::make_unique<mutable_buffers_1>(user_buf);
    cur_user_read_handler_ = user_handler;
  }
  void UnsetUserRead() {
    cur_user_read_buf_ = nullptr;
    cur_user_read_handler_ = nullptr;
  }

  //bool AtomicIsClosed() const { return closed_; }
  //void AtomicSetClosed(bool closed) { closed_ = closed; }

  void IncrementNumWritesInProgress() {
    num_writes_in_progress_ += 1;
  }
  void DecrementNumWritesInProgress() {
    num_writes_in_progress_ -= 1;
  }
  size_t GetNumWritesInProgress() const { return num_writes_in_progress_; }

private:
  boost::asio::streambuf input_stmbuf_;

  Uptr<mutable_buffers_1> cur_user_read_buf_;
  HandlerWithVlErrSize cur_user_read_handler_;

  size_t num_writes_in_progress_{ 0 };
  size_t bytes_before_drain_{ 0 };
  size_t peer_bytes_before_drain_{ 0 };

  //std::atomic<bool> closed_{ false };
};
