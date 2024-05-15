#include "co/net/asio_endpoint_from_sockaddr.h"

using namespace boost::asio::ip;

namespace co {
namespace net {

bool endpoint_from_sockaddr(
    boost::asio::ip::tcp::endpoint& ep,
    const sockaddr* name,
    int namelen)
{
  switch (name->sa_family) {
  case AF_INET:
  {
    const sockaddr_in* pa = (const sockaddr_in*)name;
    ep = tcp::endpoint(
          address_v4(htonl(pa->sin_addr.s_addr)), htons(pa->sin_port));
    return true;
  }

  case AF_INET6:
  {
    const sockaddr_in6* pa6 = (const sockaddr_in6*)name;
#ifdef _WIN32
    const unsigned char* x = (const unsigned char*)pa6->sin6_addr.u.Byte;
#elif __APPLE__
    // Darwin
    const unsigned char* x = (const unsigned char*)pa6->sin6_addr.__u6_addr.__u6_addr32;
#else
    // Linux
    const unsigned char* x = (const unsigned char*)pa6->sin6_addr.__in6_u.__u6_addr32;
#endif
    std::array<unsigned char, 16> bytes = {
      x[0], x[1], x[2],  x[3],  x[4],  x[5],  x[6],  x[7],
      x[8], x[9], x[10], x[11], x[12], x[13], x[14], x[15] // ye, ye, all ur life like this 10010 10101010 btch
    };
    ep = tcp::endpoint(address_v6(bytes), htons(pa6->sin6_port));
    return true;
  }

  default:
    return false;
  }
}

}}







