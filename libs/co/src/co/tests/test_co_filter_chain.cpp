#include "co/base/filter_chain.h"
#include "co/base/tests.h"
#include "co/xlog/xlog.h"

using namespace co;
using namespace std;
using namespace co::xlog;

namespace {
class ServerEvents {
public:
  virtual ~ServerEvents() = default;

  virtual void OnHappied(int ntimes) = 0;
  virtual void OnRecovered() = 0;
};

class ClientEvents {
public:
  virtual ~ClientEvents() = default;

  virtual void OnFoodEaten() = 0;
  virtual void OnDiedYesterday() = 0;
};

class ServerEventsLink : public FilterChainLink<ServerEvents> {
public:
  virtual ~ServerEventsLink() = default;

  void OnHappied(int ntimes) override {
    if (GetNext()) {
      GetNext()->OnHappied(ntimes);
    }
  }
  void OnRecovered() override {
    if (GetNext()) {
      GetNext()->OnRecovered();
    }
  }
};

class ClientEventsLink : public FilterChainLink<ClientEvents> {
public:
  virtual ~ClientEventsLink() = default;

  void OnFoodEaten() override {
    if (GetNext()) {
      GetNext()->OnFoodEaten();
    }
  }
  void OnDiedYesterday() override {
    if (GetNext()) {
      GetNext()->OnDiedYesterday();
    }
  }
};

// ----------------------------------------------------------------------------------------------------------------------------------

class HookedServerEvents : public ServerEventsLink {
public:
  virtual ~HookedServerEvents() = default;

  void OnHappied(int ntimes) override {
    cout << __FUNCTION__ << " ntimes = " << ntimes << "\n";
    ServerEventsLink::OnHappied(ntimes * 3);
  }
  void OnRecovered() override {
    cout << __FUNCTION__ << "\n";
    if (!false) {
      ServerEventsLink::OnRecovered();
    }
  }
};
class HookedClientEvents : public ClientEventsLink {
public:
  virtual ~HookedClientEvents() = default;

  void OnFoodEaten() override {
    ClientEventsLink::OnFoodEaten();

  }
  void OnDiedYesterday() override {
    ClientEventsLink::OnDiedYesterday();
  }
};
} //namespace `

// ----------------------------------------------------------------------------------------------------------------------------------

void test_co_filter_chain(TestInfo& ti) {

  FilterChainHead<ServerEventsLink> head;
  head.OnHappied(1);

  HookedServerEvents e1;
  HookedServerEvents e2;
  HookedServerEvents e3;

  head.AttachTop(&e1);
  head.AttachTop(&e2);
  head.OnHappied(3);

  head.AttachTop(&e3);
  head.OnRecovered();

}

void test_co_filter_chain_to_chain(TestInfo& ti) {

  FilterChainHead<ServerEventsLink> head1;
  FilterChainHead<ServerEventsLink> head2;
  head1.OnHappied(1);
  head1.OnHappied(1);

  HookedServerEvents e1;
  HookedServerEvents e2;
  HookedServerEvents e3;

  head1.AttachTop(&e1);
  head1.AttachTop(&e2);
  head1.AttachTop(&e3);

  head1.OnHappied(11);
  head1.OnRecovered();

  head1.AttachTop(&head2);
  head2.OnRecovered();
}

namespace {
class TwoBasesXxx : public HookedClientEvents, public HookedServerEvents {
public:
  virtual ~TwoBasesXxx() = default;
};
}

void test_co_filter_chain_two_bases(TestInfo& ti) {

  FilterChainHead<ClientEventsLink> chead;
  FilterChainHead<ServerEventsLink> shead;
  TwoBasesXxx xxx;
  chead.AttachTop(&xxx);
  shead.AttachTop(&xxx);
  chead.OnDiedYesterday();
  shead.OnHappied(228);
}


