#pragma once

#include "co/async/startable_stopable.h"
#include "co/async/thread_model_config.h"
#include "co/common.h"
#include <boost/asio/io_context.hpp>
#include <boost/thread.hpp>
#include <vector>

namespace co {
namespace async {

class ThreadModelGroup;

class ThreadModelBase : public ThreadsafeStopable {
public:
  virtual ~ThreadModelBase() = default;
  virtual io_context& GetDefaultIoctx() = 0;
};

class ThreadModelST : public ThreadModelBase {
public:
  virtual ~ThreadModelST() = default;
  io_context& GetDefaultIoctx() override {
    return ioc_;
  }
  void StopThreadsafe() override {
    ioc_.stop();
  }
private:
  io_context ioc_;
};

class IoctxRunner {
public:
  virtual ~IoctxRunner() = default;
  virtual void DoWrappedIoContextRun(io_context& ioc) = 0;
};

class DefaultIoctxRunner : public IoctxRunner {
public :
  virtual ~DefaultIoctxRunner() = default;
  void DoWrappedIoContextRun(io_context& ioc) override {
    ioc.run();
  }
};

class DefaultIoctxRunnerExcp : public IoctxRunner {
public:
  virtual ~DefaultIoctxRunnerExcp() = default;

  void DoWrappedIoContextRun(io_context& ioc) override {
    try {
      ioc.run();
    }
    catch (...) {
      last_excep_ = std::current_exception();
    }
  }
private:
  std::exception_ptr last_excep_;
};

// GLOBAL
extern DefaultIoctxRunner gDefaultIocRunner;

/*
* class ThreadModel
* 
* Multithreaded.
*/
class ThreadModel : public ThreadModelBase {
public:
  virtual ~ThreadModel();

  ThreadModel(const ThreadModel&) = delete;
  ThreadModel(const ThreadModelConfig& conf = ThreadModelConfig());

  io_context& GetDefaultIoctx() override;
  inline io_context& DefIOC() { return GetDefaultIoctx(); } // syntax sugar

  // if |group_num| is outside |conf.num_threads_for_group| range,
  // |conf.num_threads_default| is used
  boost::asio::io_context& AcquireIoContextForThreadGroup(size_t group_num);

  void Run(IoctxRunner& ioc_runner = gDefaultIocRunner);

  void StopThreadsafe() override;

  void Restart();

  size_t GetNumThreadsDefault() const;

private:
  void StartThreads(IoctxRunner&);
  void WaitThreads();

private:
  ThreadModelConfig conf_;
  std::map<size_t, Shptr<ThreadModelGroup>> thread_groups_;
  bool start_called_;
  Uptr<io_context> ioc_this_thrd_; // created only if conf_.force_this_thread_
  size_t num_threads_default_; // need copy for GetNumThreadsDefault()
};

class IoctxRunnerFromFunc : public IoctxRunner {
public:
  IoctxRunnerFromFunc(Func<void(io_context&)> fn_ioc_run) :
    fn_ioc_run_(fn_ioc_run)
  {

  }
  void DoWrappedIoContextRun(io_context& ioc) override {
    fn_ioc_run_(ioc);
  }
private:
  Func<void(io_context&)> fn_ioc_run_;
};

}}



