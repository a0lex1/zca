#pragma once

// This header imports common types and funcs into co namespace.
// It's also decides which type to use globally like std::function or boost::function.
// Very helpful because I don't want to write full::namespace::paths in headers.
// Some types are renamed for fancy look.
// This header can be used inside other projects that are using co.

#include "co/base/debug_tag_owner.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/system/error_code.hpp>
#include <boost/smart_ptr/atomic_shared_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/bind/bind.hpp>
#include <boost/function.hpp>

#include <shared_mutex>
#include <functional>
#include <memory>
#include <map>
#include <list>
#include <string>
#include <cstdint>
#include <cassert>

// CO_FUNCTIONAL_BOOST
// CO_SILENCE_DCHECKS

using std::size_t;

// Moved outside co:: to make code shorter
template <typename ... T> using Uptr = std::unique_ptr<T...>;
template <typename ... T> using Shptr = std::shared_ptr<T...>;
template <typename ... T> using Weakptr = std::weak_ptr<T...>;

// There is no adequate lock-free implementation of atomic shared_ptr at this moment.
// We use boost's implementation that is currently not lock-free.
// Just sit here and wait for C++ nerds to add lock-free shared_ptr to the standard.
// Now AtomicShptr<> can be only used to store/load BoostShptr<>
template <typename ... T> using AtomicShptr = boost::atomic_shared_ptr<T...>;
template <typename ... T> using BoostShptr = boost::shared_ptr<T...>;

using std::make_unique;
using std::make_shared;

#ifdef CO_FUNCTIONAL_BOOST
template <typename ... T> using Func = boost::function<T...>;
using namespace boost::placeholders;
#else
template <typename ... T> using Func = std::function<T...>;
using namespace std::placeholders;
#endif

namespace _internal {
template<typename T>
void do_release(typename boost::shared_ptr<T> const&, T*) { }
}

// Can't use std::error_code because like sock_.shutdown(boost::asio::socket_base::shutdown_both, err)
using Errcode = boost::system::error_code;

static Errcode GetErrcodeForErrno(int _ErrNo_) {
  Errcode err;
  err.assign(_ErrNo_, boost::system::system_category());
  return err;
}

using EmptyHandler = Func<void()>;
using HandlerWithErrcode = Func<void(Errcode)>;
using HandlerWithErrcodeSize = Func<void(Errcode, size_t)>;

using StringMap = std::map<std::string, std::string>;
using StringVector = std::vector<std::string>;
using StringList = std::list<std::string>;

using boost::asio::io_context;

using Strand = io_context::strand;

using SharedLock = std::shared_mutex;
using WriteLock = std::unique_lock<SharedLock>;
using ReadLock = std::shared_lock<SharedLock>;

namespace co {

// #SharedFromThisRefTrackerIssue
// boost::bind differs from std::bind by the order of DTOR of obj,args

#ifdef CO_FUNCTIONAL_BOOST
using boost::bind;
#else
using std::bind;
#endif

static inline Errcode NoError() { return Errcode(); }

// Legacy Import types, not don't define
#define tcp_acceptor boost::asio::ip::tcp::acceptor
#define tcp_socket boost::asio::ip::tcp::socket
#define tcp_endpoint boost::asio::ip::tcp::endpoint
#define ip_address boost::asio::ip::address

template <typename ... T> using enable_shared_from_this = std::enable_shared_from_this<T...>;

// boost::shared_ptr<> to std::shared_ptr<>

template<typename T>
typename std::shared_ptr<T> to_std(typename boost::shared_ptr<T> const& p) {
  return
    std::shared_ptr<T>(
      p.get(),
      boost::bind(&_internal::do_release<T>, p, boost::placeholders::_1));
}

// to enable shared_from_this() in derived class

template <typename Base>
inline std::shared_ptr<Base>
shared_from_base(std::enable_shared_from_this<Base>* base)
{
    return base->shared_from_this();
}
template <typename Base>
inline std::shared_ptr<const Base>
shared_from_base(std::enable_shared_from_this<Base> const* base)
{
    return base->shared_from_this();
}
template <typename That>
inline std::shared_ptr<That>
shared_from(That* that)
{
    return std::static_pointer_cast<That>(shared_from_base(that));
}

//#ifdef _WIN32
//#define C_vsnwprintf _vswprintf
//#else
#define C_vsnwprintf vswprintf
//#endif


} // namespace co {

// DCHECK No Error, DCHECK(!err)
#define DCHECK_NE(expr) DCHECK( ! (expr) )

#define SILENT_DCHECK(expr) if (!(expr)) { \
  printf("DCHECK %s FAILED\n", #expr); \
  abort(); \
}

#ifndef CO_SILENCE_DCHECKS
#ifndef NDEBUG
#define DCHECK assert
#else
// No assert() in release, need silent here too.
#define DCHECK(expr) SILENT_DCHECK(expr)
#endif
#else
// Silence dchecks.
#define DCHECK(expr) SILENT_DCHECK(expr)
#endif
#define NOTREACHED() { DCHECK(!"NOTREACHED"); abort(); }

#define UNREF_PARAMETER(x) (x) // < TODO: this doesn't work, need (void)(x)

//#ifndef _NDEBUG
//#define IFDEF_DEBUG(code) code
//#else
//#define IFDEF_DEBUG(...)
//#endif


// IFDEF_DEBUG( int x=5; int y=x+2; printf("%d %d\n", x, y); )

#define KEY_FOUND(x, y) (x.find(y) != x.end())

#define CO_LOWORD(l)           ((uint16_t)(((uint32_t)(l)) & 0xffff))
#define CO_HIWORD(l)           ((uint16_t)((((uint32_t)(l)) >> 16) & 0xffff))

#define CO_MAKEWORD(lb, hb) ((hb << 8 ) | lb)
#define CO_MAKEDWORD(lw, hw) ((hw << 16 ) | lw)

#define DISALLOW_COPY_AND_ASSIGN(classname) \
  classname(const classname&) {} \
  classname& operator=(const classname&) {}

// for llog() << SPTR(this) ...
#define SPTR co::string_print_ptr

#ifdef _WIN32
#define StrICmp stricmp
#define ThreadId() GetCurrentThreadId()
#define ThreadIdType DWORD
#else
#define StrICmp strcasecmp
#define ThreadIdType pthread_t
#define ThreadId() pthread_self()
#endif

// Some common routines

namespace co {

void SleepMsec(uint32_t msec);

}












