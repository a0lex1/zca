#include "co/base/hashable_weak_ptr.h"
#include "co/base/ref_tracker.h"
#include "co/xlog/xlog.h"

#include <algorithm>

// #WorkFeatures
#define CO_RT_UNORDERED_MAP // instead of std::map for RefTrackerContextDatas


using namespace std;

namespace co {

#ifdef CO_RT_TRACKING_RTCONTEXTS

#ifdef CO_RT_UNORDERED_MAP
using MapOfRTCTXDataWeaks = std::unordered_map<RefTrackerContextData*,
                                                Weakptr<RefTrackerContextData>>;
#else
using MapOfRTCTXDataWeaks = std::map<RefTrackerContextData*, Weakptr<RefTrackerContextData>>;
#endif

static boost::mutex gRTCTXDataListMutex;
// A global hashtable of RefTrackerContext-s.
static MapOfRTCTXDataWeaks gRTCtxDataMap;
#endif // CO_RT_TRACKING_RTCONTEXTS

  
// ---------------------------------------------------------------------------------------------------------

RefTrackerContextData::~RefTrackerContextData() {
  if (!disable_calls_ && rt_count_ != 0) {
    PrintLeakWarning();
  }
#ifdef CO_RT_TRACKING_RTCONTEXTS
  gRTCTXDataListMutex.lock();
  auto it = gRTCtxDataMap.find(this);
  if (it != gRTCtxDataMap.end()) {
    gRTCtxDataMap.erase(it);
  }
  gRTCTXDataListMutex.unlock();
#endif
}

Shptr<RefTrackerContextData> RefTrackerContextData::Create(const Location& loc) {
  //DCHECK(disable_calls_ || rt_count_ == 0);
  auto inst = Shptr<RefTrackerContextData>(new RefTrackerContextData(loc));
#ifdef CO_RT_TRACKING_RTCONTEXTS
  gRTCTXDataListMutex.lock();
  gRTCtxDataMap.insert(std::pair<RefTrackerContextData*, Weakptr<RefTrackerContextData>>(
                          inst.get(), inst->weak_from_this()));
  gRTCTXDataListMutex.unlock();
#endif
  return inst;
}

void RefTrackerContextData::DisableOnReleaseCalls() {
  disable_calls_ = true;
}

bool RefTrackerContextData::IsOnReleaseCallsDisabled() const {
  return disable_calls_;
}

void RefTrackerContextData::IncrementRefTrackerCount() {
  ++rt_count_;
}

void RefTrackerContextData::DecrementRefTrackerCount() {
  --rt_count_;
}

size_t RefTrackerContextData::GetAtomicRefTrackerCount() const {
  return rt_count_;
}

RefTrackerContextData::RefTrackerContextData(const Location& loc)
  : loc_(loc)
{

}

#ifndef CO_RT_TRACKING_REFTRACKERS
void RefTrackerContextData::PrintLeakWarning()
{
}
#else
std::list<RefTrackerData *> &RefTrackerContextData::_DbgGetRTDATAList() {
  return _dbg_rtdata_list_;
}

void RefTrackerContextData::PrintLeakWarning()
{
  stringstream ss;
  PrintRefTrackerContextDataList(ss);
  cout << ss.str() << "\n";
}

void RefTrackerContextData::_DbgAddTracking(RefTrackerData* p)
{
  _dbg_mutex_.lock();
  auto it = std::find(_dbg_rtdata_list_.begin(), _dbg_rtdata_list_.end(), p);
  DCHECK(it == _dbg_rtdata_list_.end());
  _dbg_rtdata_list_.push_back(p);
  _dbg_mutex_.unlock();
}

void RefTrackerContextData::_DbgRemoveTracking(RefTrackerData* p)
{
  _dbg_mutex_.lock();
  auto it = std::find(_dbg_rtdata_list_.begin(), _dbg_rtdata_list_.end(), p);
  DCHECK(it != _dbg_rtdata_list_.end());
  _dbg_rtdata_list_.erase(it);
  _dbg_mutex_.unlock();
}
#endif

// ---------------------------------------------------------------------------------------------------------

RefTracker::~RefTracker() {
  return;
}

RefTracker::RefTracker(const Location& loc,
                       RefTrackerContextHandle ctxhandle,
                       EmptyHandler on_release)
  :
  data_(new RefTrackerData(loc, ctxhandle.ctxdata_, on_release, nullptr))
{
}

RefTracker::RefTracker(const Location& loc, EmptyHandler on_release, const RefTracker& higher)
  :
  data_(new RefTrackerData(loc, higher.data_->GetContextData(), on_release, higher.data_))
{
}

RefTracker::RefTracker(const RefTracker& r) {
  *this = r;
}

RefTrackerContextHandle RefTracker::GetContextHandle(){
  return RefTrackerContextHandle(data_->GetContextData());
}

// ---------------------------------------------------------------------------------------------------------

RefTrackerContext::RefTrackerContext(const Location& loc)
  : ctxdata_(RefTrackerContextData::Create(loc))
{

}

// ---------------------------------------------------------------------------------------------------------

RefTrackerContextHandle::RefTrackerContextHandle(Shptr<RefTrackerContextData> ctxdata) {
  ctxdata_ = ctxdata;
}

co::RefTrackerContextHandle RefTrackerContext::GetHandle() {
  return RefTrackerContextHandle(ctxdata_);
}

void RefTrackerContext::DisableOnReleaseCalls() {
  ctxdata_->DisableOnReleaseCalls();
}

bool RefTrackerContext::IsOnReleaseCallsDisabled() const {
  return ctxdata_->IsOnReleaseCallsDisabled();
}

size_t RefTrackerContext::GetAtomicRefTrackerCount() const {
  return ctxdata_->GetAtomicRefTrackerCount();
}

void RefTrackerContext::_DbgLogPrintTrackedRefTrackers()
{
#ifdef CO_RT_TRACKING_REFTRACKERS
  if (ctxdata_->_DbgGetRTDATAList().size()) {
    ctxdata_->PrintLeakWarning();
  }
#endif
}

// -----------------------------------------------------------------------------------------------------------------------

RefTrackerData::RefTrackerData(Shptr<RefTrackerContextData> ctxdata,
                               EmptyHandler on_release,
                               Shptr<RefTrackerData> higher_data)
  :
    RefTrackerData(Location(), ctxdata, on_release, higher_data)
{
}


RefTrackerData::RefTrackerData(const Location &loc,
                               Shptr<RefTrackerContextData> ctxdata,
                               EmptyHandler on_release,
                               Shptr<RefTrackerData> higher_data)
  :
    ctxdata_(ctxdata),
    src_location_(loc),
    on_release_(on_release),
    higher_data_(higher_data)
{
  ctxdata_->IncrementRefTrackerCount();
  ctxdata_->_DbgAddTracking(this);
}


RefTrackerData::~RefTrackerData() {
  ctxdata_->DecrementRefTrackerCount();
  if (!ctxdata_->IsOnReleaseCallsDisabled()) {
    // CALL USER'S |on_release| CALLBACK
    on_release_();
  }
  // #ReleaseRule1
  // Now destroy on_release_ so its closures destroyed too
  // important to do this before releasing |higher_data|
  // if we not doing this, the order of class member definition rules the order
  on_release_ = nullptr;

  // #SharedFromThisRefTrackerIssue
  extra_obj_bound_ = nullptr;

  ctxdata_->_DbgRemoveTracking(this);
  // |higher_data| will be released
}

Shptr<RefTrackerContextData> RefTrackerData::GetContextData() {
  return ctxdata_;
}

#ifdef CO_RT_TRACKING_RTCONTEXTS
void GetRefTrackerContextDataList(std::unordered_set<Shptr<RefTrackerContextData>>& datas) {
  // |datas| gets only what's lock()ed
  gRTCTXDataListMutex.lock();
  for (auto& entry: gRTCtxDataMap) {
    Shptr<RefTrackerContextData> data = entry.second.lock();
    if (data) {
      datas.insert(data);
    }
  }
  gRTCTXDataListMutex.unlock();
}

void PrintRefTrackerContextDataList(std::ostream &ss) {
#if defined(CO_RT_TRACKING_REFTRACKERS) && defined(CO_RT_TRACKING_RTCONTEXTS)
  std::unordered_set<Shptr<RefTrackerContextData>> ctxdatas;
  GetRefTrackerContextDataList(ctxdatas);
  ss << "[ ] Dumping RefTrackerContextData context(s) data(s)\n";
  size_t ctd_index = 0;
  for (auto& ctxdata: ctxdatas) {
    ss << "  [" << ctd_index << "] Dumping RefTracker-s for context data (atomic count = " << ctxdata->GetAtomicRefTrackerCount() << ")\n"
       << "  <Location: " << ctxdata->_DbgGetLocation().ToString() << ">\n";
    size_t index = 0;
    for (auto& rtdata: ctxdata->_DbgGetRTDATAList()) {
      ss << "    [ " << index << " ] " << rtdata->GetLocation().GetFile() << ":"
         << rtdata->GetLocation().GetLine() << " func " << rtdata->GetLocation().GetFunc() << "\n";
      index += 1;
    }
    ctd_index += 1;
  }
  ss << "The end of RefTrackers\n";

  //printf("%s\n", ss.str().c_str());

#else
  outp = "Need to be compiled with both CO_RT_TRACKING_RTCONTEXTS and CO_RT_TRACKING_REFTRACKERS";
#endif
}
#endif

}
















