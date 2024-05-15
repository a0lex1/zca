#pragma once

#include "co/base/adaptor.h"

#include "co/common.h"

#include <boost/coroutine/all.hpp>

// Nerd magic from stackoverflow
template <int>
struct variadic_placeholder {};
namespace std {
  template <int N>
  struct is_placeholder<variadic_placeholder<N>>
    : integral_constant<int, N + 1>
  {
  };
}

namespace co {
// Nerd magic from stackoverflow
template <typename Ret, typename Class, typename... Args, size_t... Is, typename... Args2>
auto my_bind(std::index_sequence<Is...>, Ret(Class::* fptr)(Args...), Args2&&... args) {
  return std::bind(fptr, std::forward<Args2>(args)..., variadic_placeholder<Is>{}...);
}
template <typename Ret, typename Class, typename... Args, typename... Args2>
auto my_bind(Ret(Class::* fptr)(Args...), Args2&&... args) {
  return my_bind(std::make_index_sequence<sizeof...(Args) - sizeof...(Args2) + 1>{}, fptr, std::forward<Args2>(args)...);
}

// -------------------------------------------------------------------------

// ADAPTOR

class AsyncCoro;

template <typename T>
class AsyncCoroAdaptor : public Adaptor<T> {
public:
  virtual ~AsyncCoroAdaptor() = default;

  AsyncCoroAdaptor(Uptr<T> adapted_uptr, AsyncCoro& coro)
    :Adaptor<T>(std::move(adapted_uptr)), coro_(coro)
  {
  }
  AsyncCoroAdaptor(T& adapted_ref, AsyncCoro& coro)
    :Adaptor<T>(adapted_ref), coro_(coro)
  {
  }
protected:
  AsyncCoro& GetCoro() {
    return coro_;
  }
private:
  AsyncCoro& coro_;
};

// -------------------------------------------------------------------------

template <typename... Args>
class AsyncCoroContinuationCallback;
// CORO

class AsyncCoro {
public:
  template <typename ... T> using coro = boost::coroutines::coroutine<T...>;
  typedef coro<void> coro_t;
  typedef coro_t::push_type push_t;
  typedef coro_t::pull_type pull_t;

  typedef Func<void()> user_coro_func_t;

  AsyncCoro(user_coro_func_t user_func = user_coro_func_t()):
    user_func_(user_func),
    coro_(my_bind(&AsyncCoro::CoroEntry, this, _1)),
    pull_(nullptr)
  {
  }
  // If not set from CTOR
  void SetUserFunction(user_coro_func_t user_func) {
    user_func_ = user_func;
  }
  void Enter() {
    coro_();
    if (new_io_) {
      // Initiate requested I/O after coroutine yields
      auto io_copy(new_io_);
      new_io_ = decltype(new_io_)(); // clear
      io_copy();
    }
  }

private:
  user_coro_func_t user_func_;
  push_t coro_;
  pull_t* pull_;
  Func<void()> new_io_;

private:
  void CoroEntry(pull_t& pull) {
    assert(pull_ == nullptr);
    pull_ = &pull;
    user_func_();
    pull_ = nullptr;
  }

public:
  template <typename... Args>
  AsyncCoroContinuationCallback<Args...> CreateContinuationCallback() {
    return AsyncCoroContinuationCallback<Args...>(*this);
  }
  void DoYield(Func<void()> new_io = nullptr) {
    DCHECK(!new_io_);
    new_io_ = new_io;
    (*pull_)();
  }
};

// -------------------------------------------------------------------------

// CONTINUATION CALLBACK

template <typename... Args>
class AsyncCoroContinuationCallback {
public:
  template <typename ... T> using coro = boost::coroutines::coroutine<T...>;
  typedef coro<void> coro_t;
  typedef coro_t::push_type push_t;
  typedef coro_t::pull_type pull_t;

  AsyncCoroContinuationCallback(AsyncCoro& host_coro)
    : host_coro_(host_coro)
  {
    funct_ = my_bind(
      &std::remove_reference<decltype(*this)>::type::CbkHandler,
      this);
  }
  auto GetFunction() const {
    return funct_;
  }
  const std::tuple<Args...>& ResultTuple() const {
    return result_tp_;
  }
private:
  void CbkHandler(Args... args) {
    result_tp_ = std::tuple<Args...>(args...);

    // REENTER
    host_coro_.Enter();
  }
  friend class AsyncCoro;
private:
  AsyncCoro& host_coro_;
  Func<void(Args... args)> funct_;
  std::tuple<Args...> result_tp_;
};

/*

#ifdef _WIN32

// Today's crossplatformability breaking news on CNN
// how are u dear MSVC?
#define CREATE_CONTINUATION_CALLBACK1(cor, ArgType1) \
  cor.CreateContinuationCallback<ArgType1>()
#define CREATE_CONTINUATION_CALLBACK2(cor, ArgType1, ArgType2) \
  cor.CreateContinuationCallback<ArgType1, ArgType2>()

#else

// GCC :-*
#define CREATE_CONTINUATION_CALLBACK1(cor, ArgType1) \
  cor.CreateContinuationCallback<ArgType1>(ArgType1)

#define CREATE_CONTINUATION_CALLBACK2(cor, ArgType1, ArgType2) \
  cor.CreateContinuationCallback<ArgType1, ArgType2>(ArgType1, ArgType2)
// love u

#endif

// love both of u guys, don't be like this
*/

}


