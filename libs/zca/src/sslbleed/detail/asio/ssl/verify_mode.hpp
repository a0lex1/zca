#pragma once

#define 	SSL_VERIFY_NONE   0x00
#define 	SSL_VERIFY_PEER   0x01
#define 	SSL_VERIFY_FAIL_IF_NO_PEER_CERT   0x02
#define 	SSL_VERIFY_CLIENT_ONCE   0x04


namespace asio {
namespace ssl {

typedef int verify_mode;

const int verify_none = SSL_VERIFY_NONE;
const int verify_peer = SSL_VERIFY_PEER;
const int verify_fail_if_no_peer_cert = SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
const int verify_client_once = SSL_VERIFY_CLIENT_ONCE;

} // namespace ssl
} // namespace asio

