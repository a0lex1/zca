#include "co/base/tests.h"
#include "co/base/async_coro.h"
#include "co/xlog/xlog.h"

using namespace std;
using namespace co;

//#define CAUSE_CORO0_REENTER_CRASH

void test_co_async_coro0(TestInfo& test_info) {
  Func<void()> cbkfunc;
  AsyncCoro coro;
  bool reentered = false;
  coro.SetUserFunction([&]() {
    auto cbk = coro.CreateContinuationCallback<>();
    cbkfunc = cbk.GetFunction();
    // Waiting for user to call our continuation callback
#ifdef CAUSE_CORO0_REENTER_CRASH
    cbkfunc(); // REENTER CRASH!
#endif
    coro.DoYield();
    reentered = true;
                        });
  coro.Enter();
  cbkfunc();
  DCHECK(reentered);
}

void test_co_async_coro1(TestInfo& test_info) {
  Func<void(Errcode)> cbkfunc;
  Errcode x1;
  AsyncCoro coro;
  coro.SetUserFunction([&]() {
    //auto cbk = coro.CreateContinuationCallback<Errcode>(coro, Errcode);
    AsyncCoroContinuationCallback<Errcode> cbk =
        coro.CreateContinuationCallback<Errcode>();
    cbkfunc = cbk.GetFunction();
    // Waiting for user to call our continuation callback
    coro.DoYield();
    x1 = std::get<0>(cbk.ResultTuple());
                        });
  coro.Enter();
  Errcode myerr = boost::asio::error::address_family_not_supported;
  cbkfunc(myerr);
  syslog(_INFO) << "x1 == " << x1 << "\n";
  DCHECK(x1 == myerr);
}

void test_co_async_coro2(TestInfo& test_info) {
  Func<void(Errcode, int)> cbkfunc;
  Errcode x1;
  int x2;
  AsyncCoro coro;
  coro.SetUserFunction([&]() {
    auto cbk = coro.CreateContinuationCallback<Errcode, int>();
    cbkfunc = cbk.GetFunction();
    // Waiting for user to call our continuation callback
    coro.DoYield();
    x1 = std::get<0>(cbk.ResultTuple());
    x2 = std::get<1>(cbk.ResultTuple());
                        });
  coro.Enter();
  Errcode myerr = boost::asio::error::address_family_not_supported;
  cbkfunc(myerr, 1337);
  syslog(_INFO) << "x1 == " << x1 << ", x2 == " << x2 << "\n";
  DCHECK(x1 == myerr);
  DCHECK(x2 == 1337);
}









