#pragma once

// class Endpoint + class TcpEndpoint

#include "co/net/asio_endpoint_from_string.h"
#include "co/base/strings.h"
#include "co/common.h"
#include <boost/asio/ip/tcp.hpp>
#include <sstream>

namespace co {
namespace net {

namespace detail {
static const int CO_ENDPOINT_ID_TCP = 765;
}

class Endpoint;
class EndpointImpl {
 public:
  EndpointImpl() {}
  virtual ~EndpointImpl() = default;

 protected:
  friend class Endpoint;
  virtual int GetType() const = 0;
  virtual std::string ToString() const = 0;
  virtual void FromString(const std::string&, Errcode& err) = 0;
  virtual bool Compare(const EndpointImpl&) const = 0;
};

// Facade class
class Endpoint {
 public:
  Endpoint() {}
  Endpoint(Shptr<EndpointImpl> impl) : impl_(impl) {}

  template <typename T>
  const Shptr<T> GetImpl(int type_required) const {
    DCHECK(impl_);
    DCHECK(impl_->GetType() == type_required);
    return std::static_pointer_cast<T>(impl_);
  }

  template <typename T>
  Shptr<T> GetImpl(int type_required) {
    DCHECK(impl_);
    DCHECK(impl_->GetType() == type_required);
    return const_cast<const Endpoint*>(this)->GetImpl<T>(type_required);
  }

  int GetType() const { return impl_->GetType(); }

  std::string ToString() const {
    if (impl_ == nullptr) {
      return "";
    }
    return impl_->ToString();
  }

  void FromString(const std::string& s, Errcode& err) {
    DCHECK(impl_);
    impl_->FromString(s, err);
  }

  // Pukers, you know them already by my previous comments. Puk-puk.
  void FromString(const std::string& s) {
    Errcode err;
    FromString(s, err);
    if (err) {
      BOOST_THROW_EXCEPTION(boost::system::system_error(err));
    }
  }

  bool operator==(const Endpoint& r) {
    DCHECK(impl_);
    if (this->impl_ == nullptr && r.impl_ == nullptr) {
      // both nullptr impls
      return true;
    }
    return impl_->Compare(*r.impl_.get());
  }

  bool operator!=(const Endpoint& r) {
    return !operator==(r);
  }

 private:
  Shptr<EndpointImpl> impl_;
};

//
// --- TCP ---
//

class TcpEndpointImpl : public EndpointImpl {
 public:
  virtual ~TcpEndpointImpl() = default;

  TcpEndpointImpl() {}
  TcpEndpointImpl(boost::asio::ip::tcp::endpoint addr) : asio_ep_(addr) {}

  boost::asio::ip::tcp::endpoint GetAddr() const { return asio_ep_; }

 private:
  int GetType() const override { return detail::CO_ENDPOINT_ID_TCP; }

  std::string ToString() const override {
    std::stringstream ss;
    ss << asio_ep_;
    return ss.str();
  }

  void FromString(const std::string& ipport_str, Errcode& err) override {
    net::EndpointFromIpPortStr(asio_ep_, ipport_str, err);
  }

  bool Compare(const EndpointImpl& rimpl) const override {
    const auto& trimpl = static_cast<const TcpEndpointImpl&>(rimpl);
    DCHECK(this->GetType() == trimpl.GetType());
    return this->asio_ep_ == trimpl.asio_ep_;
  }

 private:
  boost::asio::ip::tcp::endpoint asio_ep_;
};

class TcpEndpoint : public Endpoint
{
 public:
  TcpEndpoint(const std::string& ip_or_ipport)
    : TcpEndpoint(net::EndpointFromIpPortStr(ip_or_ipport))
  {
  }
  TcpEndpoint(const std::string& ip_addr, uint16_t port)
    : TcpEndpoint(tcp_endpoint(ip_address::from_string(ip_addr), port))
  {
  }
  TcpEndpoint()
    : Endpoint(make_shared<TcpEndpointImpl>())
  {
  }
  TcpEndpoint(boost::asio::ip::tcp::endpoint addr)
    : Endpoint(make_shared<TcpEndpointImpl>(addr))
  {
  }
  //TcpEndpoint(Endpoint base) : Endpoint(base.GetImpl<TcpEndpointImpl>(CO_ENDPOINT_ID_TCP)) {}
  TcpEndpoint(Endpoint base) // make GCC happy
    : Endpoint(base.GetImpl<TcpEndpointImpl>(detail::CO_ENDPOINT_ID_TCP)) 
  {
  }
  boost::asio::ip::tcp::endpoint GetAddr() const { return GetImpl<TcpEndpointImpl>(detail::CO_ENDPOINT_ID_TCP)->GetAddr(); }
  static TcpEndpoint Loopback(uint16_t port = 0) {
    return TcpEndpoint(tcp_endpoint(ip_address::from_string("127.0.0.1"), port));
  }
  static TcpEndpoint NullAddress(uint16_t port = 0) {
    return TcpEndpoint(tcp_endpoint(ip_address::from_string("0.0.0.0"), port));
  }
};

}}


