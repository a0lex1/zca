#pragma once

#include "./endpoint.h"
#include "./vlan_common.h"

#include <boost/asio/buffer.hpp>

class VlanNativeApi {
public:
  virtual ~VlanNativeApi() = default;

  using mutable_buffers_1 = boost::asio::mutable_buffers_1;
  using const_buffers_1 = boost::asio::const_buffers_1;

  // Writes don't need ioc_cbk because writing is on the transport's shoulders

  virtual void VnReserveAddressTSafe(const VlanEndpoint& ep, VlanError& vlerr) = 0;
  virtual void VnReleaseAddressTSafe(const VlanEndpoint& ep) = 0;
  virtual void VnAccept(vlhandle_t& handle, const VlanEndpoint& ep, HandlerWithVlErr handler, io_context& ioc_cbk) = 0;
  virtual void VnConnect(vlhandle_t& handle, const VlanEndpoint& ep, HandlerWithVlErr handler, io_context& ioc_cbk) = 0;
  virtual void VnRead(vlhandle_t handle, mutable_buffers_1 buf, HandlerWithVlErrSize handler, io_context&) = 0;
  virtual void VnWrite(vlhandle_t handle, const_buffers_1 buf, HandlerWithVlErrSize) = 0;
  virtual void VnCancelAccept(vlhandle_t handle, VlanError& vlerr) = 0;
  virtual void VnShutdownSend(vlhandle_t handle, VlanError& vlerr) = 0;
  virtual void VnClose(vlhandle_t handle) = 0;
};


