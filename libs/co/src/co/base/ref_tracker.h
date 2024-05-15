#pragma once

#include "co/base/location.h"
#include "co/common.h"

#include <boost/thread/mutex.hpp>
#include <boost/noncopyable.hpp>
#include <unordered_set>

// #WorkFeatures
//   #define CO_RT_TRACKING_REFTRACKERS // list of rts inside rtctx
//   #define CO_RT_TRACKING_RTCONTEXTS // global list of rtctx-es

namespace co {

class RefTrackerContextData;
class RefTrackerData;
class RefTrackerContext;
class RefTracker;

class RefTrackerData {
public:
  RefTrackerData(Shptr<RefTrackerContextData> ctxdata,
                 EmptyHandler on_release,
                 Shptr<RefTrackerData> higher_data = nullptr);

  RefTrackerData(const Location& loc,
                 Shptr<RefTrackerContextData> ctxdata,
                 EmptyHandler on_release,
                 Shptr<RefTrackerData> higher_data = nullptr);

  ~RefTrackerData();

  const Location& GetLocation() { return src_location_; }

  Shptr<RefTrackerContextData> GetContextData();

  friend class RefTrackerContextData;
  friend class RefTracker;

private:
  template <typename T>
  void SetReferencedObject(Shptr<T> obj) {
    extra_obj_bound_ = [obj]() { };
  }
private:
  Shptr<RefTrackerContextData> ctxdata_;
  EmptyHandler on_release_;
  Shptr<RefTrackerData> higher_data_;
  Func<void()> extra_obj_bound_;

  Location src_location_;
};


class RefTrackerContextData : public co::enable_shared_from_this<RefTrackerContextData> {
public:
  // * This adds all RefTrackerContext-s to a global table, macro *
  ~RefTrackerContextData();
  // CTOR is private. Use this.
  static Shptr<RefTrackerContextData> Create(const Location& loc);

  const Location& _DbgGetLocation() { return loc_; }

  void DisableOnReleaseCalls();
  bool IsOnReleaseCallsDisabled() const;
  void IncrementRefTrackerCount();
  void DecrementRefTrackerCount();
  size_t GetAtomicRefTrackerCount() const;

  void PrintLeakWarning();

  // * This adds RefTrackers to the list inside RefTrackerContext, micro */
#ifdef CO_RT_TRACKING_REFTRACKERS
  std::list<RefTrackerData*> & _DbgGetRTDATAList();
#endif

private:
  RefTrackerContextData(const Location&);

  friend class RefTrackerData;
#ifdef CO_RT_TRACKING_REFTRACKERS
  void _DbgAddTracking(RefTrackerData* p);
  void _DbgRemoveTracking(RefTrackerData* p);
#else
  void _DbgAddTracking(RefTrackerData* p) {}
  void _DbgRemoveTracking(RefTrackerData* p) {}
#endif

private:
  bool disable_calls_{ false };
  std::atomic<size_t> rt_count_{ 0 };

  Location loc_;

#ifdef CO_RT_TRACKING_REFTRACKERS
  std::list<RefTrackerData*> _dbg_rtdata_list_;
  boost::mutex _dbg_mutex_;
#endif
};


class RefTrackerContextHandle
{
public:
  // no public methods
private:
  friend class RefTrackerContext;
  friend class RefTracker;
  RefTrackerContextHandle(Shptr<RefTrackerContextData> ctxdata);
private:
  Shptr<RefTrackerContextData> ctxdata_;
};

class RefTracker {
public:
  ~RefTracker();

  //RefTracker(RefTrackerContextHandle ctxhandle, EmptyHandler on_release);
  RefTracker(const Location&, RefTrackerContextHandle ctxhandle, EmptyHandler on_release);

  //RefTracker(EmptyHandler on_release, const RefTracker& higher);
  RefTracker(const Location&, EmptyHandler on_release, const RefTracker& higher);

  RefTracker(const RefTracker& r);

  RefTracker() {}

  // Helps to reference shared_from_this() instances
  template <typename T>
  void SetReferencedObject(Shptr<T> obj) {
    data_->SetReferencedObject(obj);
  }

  bool IsEmpty() const { return data_ == nullptr; }
  RefTrackerContextHandle GetContextHandle();

private:

private:
  Shptr<RefTrackerData> data_;
};

class RefTrackerContext: public boost::noncopyable
{
public:
  RefTrackerContext(const Location& loc);

  RefTrackerContextHandle GetHandle();
  void DisableOnReleaseCalls();
  bool IsOnReleaseCallsDisabled() const;
  size_t GetAtomicRefTrackerCount() const;

  void _DbgLogPrintTrackedRefTrackers();

private:
  Shptr<RefTrackerContextData> ctxdata_;
};

#ifdef CO_RT_TRACKING_RTCONTEXTS
// Thread-safe
void GetRefTrackerContextDataList(std::unordered_set<Shptr<RefTrackerContextData>>& );
void PrintRefTrackerContextDataList(std::ostream& ostm);
#else
static void GetRefTrackerContextDataList(std::unordered_set<Shptr<RefTrackerContextData>>& ) {
  // Nothing
}
static void PrintRefTrackerContextDataList(std::osteam& ostm) {
  // Nothing
}
#endif

}


































