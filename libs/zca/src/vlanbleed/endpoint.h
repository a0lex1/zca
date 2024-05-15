#pragma once

#include "co/net/endpoint.h"
#include "co/base/strings.h"

#include <sstream>

class VlanAddress {
public:
  VlanAddress(uint32_t hostv = 0) : hostv_(hostv) {}

  static VlanAddress Loopback() { return VlanAddress(-1); }

  uint32_t Host() const { return hostv_; }
  uint32_t& Host() {
    return const_cast<uint32_t&>(static_cast<const VlanAddress*>(this)->hostv_);
  }


  std::string ToString() const {
    return co::string_printf("%d", hostv_);
  }
  bool FromString(const std::string& s) {
    return co::string_to_uint(s.c_str(), hostv_);
  }
  //void FromString(const std::string& s) { // throws exceptions
  //  if (!FromString(s)) {
  //    BOOST_THROW_EXCEPTION(std::exception("VlanEndpoint::FromString() failed"));
  //  }
  //}

  bool operator==(const VlanAddress& r) const {
    return this->hostv_ == r.hostv_;
  }
  bool operator!=(const VlanAddress& r) const {
    return !operator==(r);
  }

private:
  uint32_t hostv_{ 0 };
};

class VlanEndpointImpl : public co::net::EndpointImpl {
public:
  virtual ~VlanEndpointImpl() = default;

  VlanEndpointImpl() {}
  VlanEndpointImpl(const VlanAddress& host, uint32_t port) : addr_host_(host), addr_port_(port) {}

  VlanAddress GetAddress() const { return addr_host_; }
  uint32_t GetPort() const { return addr_port_; }

  // May be virtual in future (in Endpoint::)
  bool IsEqual(const VlanEndpointImpl& r) {
    return this->addr_host_ == r.addr_host_ && this->addr_port_ == r.addr_port_;
  }
private:
  int GetType() const override { return 1728389123; }
  std::string ToString() const override {
    std::stringstream ss;
    ss << std::dec << addr_host_.ToString() << ":" << addr_port_;
    return ss.str();
  }
  void FromString(const std::string& s, Errcode& err) override {
    StringVector parts;
    co::string_split(s, ":", parts);

    err = std::make_error_code(std::errc::invalid_argument);

    if (parts.size() != 2) {
      return;
    }
    uint32_t h;
    uint32_t p;
    if (!co::string_to_uint(parts[0], h) ||
        !co::string_to_uint(parts[1], p)) {
      return;
    }
    addr_host_.Host() = h;
    addr_port_ = p;
    err = co::NoError();
  }
  bool Compare(const EndpointImpl& rimpl) const override {
    const auto& trimpl = static_cast<const VlanEndpointImpl&>(rimpl);
    return this->addr_host_ == trimpl.addr_host_ &&
      this->addr_port_ == trimpl.addr_port_;
  }

private:
  VlanAddress addr_host_{ 0 };
  uint32_t addr_port_{ 0 };
};

class VlanEndpoint : public co::net::Endpoint
{
public:
  VlanEndpoint()
    :
    Endpoint(make_shared<VlanEndpointImpl>())
  {
  }
  VlanEndpoint(const VlanAddress& addr, uint32_t port)
    :
    Endpoint(make_shared<VlanEndpointImpl>(addr, port))
  {
  }
  VlanEndpoint(Endpoint base)
    :
    Endpoint(base.GetImpl<VlanEndpointImpl>(1728389123))
  {
  } // make GCC happy
  VlanAddress GetAddress() const {
    return GetImpl<VlanEndpointImpl>(1728389123)->GetAddress();
  }
  uint32_t GetPort() const {
    return GetImpl<VlanEndpointImpl>(1728389123)->GetPort();
  }
  static VlanEndpoint Loopback(uint16_t port = 0) {
    return VlanEndpoint(VlanAddress::Loopback(), port);
  }
  static VlanEndpoint NullAddress(uint16_t port = 0) {
    return VlanEndpoint(VlanAddress(0), port);
  }

  bool operator==(const VlanEndpoint& r) const {
    return this->GetImpl<VlanEndpointImpl>(1728389123)->IsEqual(
      *r.GetImpl<VlanEndpointImpl>(1728389123).get());
  }
  bool operator!=(const VlanEndpoint& r) const {
    return !operator==(r);
  }

  // Only in VlanEndpoint
  bool operator<(const VlanEndpoint& r) const {
    // compare hosts, then ports (less significant)
    if (this->GetAddress().Host() < r.GetAddress().Host()) {
      return true;
    }
    if (this->GetAddress().Host() > r.GetAddress().Host()) {
      return false;
    }
    // hosts are equal
    return (this->GetPort() < r.GetPort());
  }
};


