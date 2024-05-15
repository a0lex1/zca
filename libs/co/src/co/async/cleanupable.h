#pragma once

namespace co {
namespace async {

// class Cleanupable
//
//
class Cleanupable {
public:
  virtual ~Cleanupable() = default;

  // When ioc is stopped, RefTrackers still exists on rtctx. Proper way to handle
  // it is to rtctx.DisableOnReleaseCalls(). But if we do so, there may be a memleak.
  // Need to CleanupAbortedStop() after ioc/tm run/Run is returned.

  virtual void CleanupAbortedStop() = 0;
};

}}

