#pragma once

// https://stackoverflow.com/questions/16240716/why-no-strandwrap-equivalent-for-strandpost

// Only for strands, doesn't support posting to io_context

#include "co/common.h"

#include <boost/bind/bind.hpp>
#include <boost/asio.hpp>

#include <iostream>

namespace co {
namespace async {

/// @brief Custom handler wrapper type that will post into its dispatcher.
template <typename Dispatcher,
  typename Handler>
  class post_handler
{
public:
  using result_type = void;

  post_handler(Dispatcher dispatcher, Handler handler)
    : dispatcher_(dispatcher),
    handler_(handler)
  {}

  void operator()()
  {
    dispatcher_.post(handler_);
  }

  template <typename Arg1>
  void operator()(Arg1 arg1)
  {
    dispatcher_.post(co::bind(handler_, arg1));
  }

  template <typename Arg1, typename Arg2>
  void operator()(Arg1 arg1, Arg2 arg2)
  {
    dispatcher_.post(co::bind(handler_, arg1, arg2));
  }

  Dispatcher dispatcher_;
  Handler handler_;
};

// Custom invocation hooks for post_handler.  These must be declared in 
// post_handler's associated namespace for proper resolution.

template <typename Function, typename Dispatcher, typename Handler>
inline void asio_handler_invoke(Function& function,
                                post_handler<Dispatcher, Handler>* this_handler)
{
  this_handler->dispatcher_.post(
    boost::asio::detail::rewrapped_handler<Function, Handler>(
    function, this_handler->handler_));
}

template <typename Function, typename Dispatcher, typename Handler>
inline void asio_handler_invoke(const Function& function,
                                post_handler<Dispatcher, Handler>* this_handler)
{
  this_handler->dispatcher_.post(
    boost::asio::detail::rewrapped_handler<Function, Handler>(
    function, this_handler->handler_));
}

/// @brief Factory function used to create handlers that post through the
///        dispatcher.
template <typename Dispatcher, typename Handler>
post_handler<Dispatcher, Handler>
wrap_post(Dispatcher dispatcher, Handler handler)
{
  return post_handler<Dispatcher, Handler>(dispatcher, handler);
}

/// @brief Convenience factory function used to wrap handlers created from
///        strand.wrap.
template <typename Dispatcher, typename Handler>
post_handler<Dispatcher,
  boost::asio::detail::wrapped_handler<Dispatcher, Handler> >
  wrap_post(boost::asio::detail::wrapped_handler<Dispatcher, Handler> handler)
{
  return wrap_post(handler.dispatcher_, handler);
}

// Commented this out:

/*
boost::asio::io_service io_service;
boost::asio::Strand strand(io_service);
boost::asio::deadline_timer timer(io_service);

void a() { std::cout << "a" << std::endl; }
void b() { std::cout << "b" << std::endl; }
void c() { std::cout << "c" << std::endl; }
void d() { std::cout << "d" << std::endl; }
void noop() {}

void my_great_function()
{
  std::cout << "++my_great_function++" << std::endl;
  // Standard dispatch.
  strand.dispatch(&a);

  // Direct wrapping.
  wrap_post(strand, &b)();

  // Convenience wrapping.
  //wrap_post(strand.wrap(&c))();

  // ADL hooks.
  timer.async_wait(wrap_post(strand, co::bind(&d)));
  timer.cancel();
  std::cout << "--my_great_function--" << std::endl;
}

int main()
{
  // Execute my_great_function not within a strand.  The noop
  // is used to force handler invocation within strand.
  io_service.post(&my_great_function);
  strand.post(&noop);
  io_service.run();
  io_service.reset();

  // Execute my_great_function within a strand.
  std::cout << std::endl;
  io_service.post(strand.wrap(&my_great_function));
  strand.post(&noop);
  io_service.run();
}*/

}}


