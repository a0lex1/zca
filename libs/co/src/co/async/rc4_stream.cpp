#include "co/async/rc4_stream.h"

using namespace std;

namespace co {
namespace async {

void Rc4Stream::AsyncReadSome(
  boost::asio::mutable_buffers_1 buf, HandlerWithErrcodeSize handler)
{
  // read and decrypt

  GetUnderlyingStream().AsyncReadSome(buf, [&, handler, buf](Errcode err, size_t num_bytes) {
    if (!err) {
      DCHECK(num_bytes <= buf.size());
      r_rc4_.crypt(buf.data(), num_bytes, buf.data());
    }
    handler(err, num_bytes);
    });
}

void Rc4Stream::AsyncWriteSome(boost::asio::const_buffers_1 buf,
  HandlerWithErrcodeSize handler)
{
  // encrypt and write

  Shptr<string> tmpbuf = make_shared<string>(static_cast<const char*>(buf.data()), buf.size());
  w_rc4_.crypt(tmpbuf->c_str(), tmpbuf->length(), const_cast<char*>(tmpbuf->c_str()));

  boost::asio::const_buffers_1 newbuf(tmpbuf->c_str(), tmpbuf->length());

  GetUnderlyingStream().AsyncWriteSome(newbuf, [tmpbuf, handler](Errcode err, size_t num_bytes) {
    handler(err, num_bytes);
    });
}

}}

