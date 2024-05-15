#include "./ssl.h"
#include <mbedtls/md5.h>

#include "co/async/service.h"
#include "co/async/startable_stopable.h"
#include "co/base/cmdline/make_raw_argv.h"
#include "co/base/cmdline/parse_cmdline_to_argv.h"
#include "co/base/async_coro.h"
#include "co/async/test_kit/test_session.h"
#include "co/async/server.h"
#include "co/async/thread_model.h"
#include "co/base/tests.h"
#include "co/base/strings.h"
#include "co/async/tcp.h"
#include "co/xlog/configs.h"
#include "co/xlog/xlog.h"

#include <boost/asio/deadline_timer.hpp>

using namespace co;
using namespace co::async;
using namespace std;
using namespace co::xlog;
using namespace co::xlog::configs;
using namespace boost::asio;
using namespace boost::posix_time;
using co::net::Endpoint;
using co::net::TcpEndpoint;

#define mbedtls_printf       printf
#define mbedtls_exit         exit
#define MBEDTLS_EXIT_SUCCESS EXIT_SUCCESS
#define MBEDTLS_EXIT_FAILURE EXIT_FAILURE

static void hashLesbians();


void test_ssl_simple(TestInfo& ti) {
  hashLesbians();
  
  ThreadModel tm;
  auto sslctx = make_shared<SslContext>(false/*server*/);

  Uptr<TcpStreamAcceptor> tcp_acpt(make_unique<TcpStreamAcceptor>(tm.DefIOC()));
  Uptr<TcpStreamFactory> tcp_stm_fac(make_unique<TcpStreamFactory>(tm.DefIOC()));

  auto locaddr = TcpEndpoint::Loopback(8000);
  cout << "Starting server on " << locaddr.ToString() << "\n";
  ServerWithSessList server(locaddr,
                            ServerObjects(make_shared<SslStreamFactory>(move(tcp_stm_fac)),
                              make_unique<SslStreamAcceptor>(move(tcp_acpt), sslctx),
                              tm.DefIOC()),
                            [](Uptr<Stream> s, Shptr<Strand> sess_strand) {
                              return make_shared<test_kit::TestServerSession>(
                                move(s), sess_strand,
                                test_kit::TestSessionParams::WriteAndDisconnect(256, 'x'));
                            });

  server.SetupAcceptorNowNofail();

  RefTrackerContext rtctx(CUR_LOC());
  server.Start(RefTracker(CUR_LOC(), rtctx.GetHandle(), []() {
    cout << "SSL Server stopped\n";
               }));

  tm.Run();

}

static void hashLesbians() {
  int i, ret;
  unsigned char digest[16];
  char str[] = "Hello, world!";
  mbedtls_printf("\n  MD5('%s') = ", str);
  if ((ret = mbedtls_md5((unsigned char*)str, 13, digest)) != 0)
    mbedtls_exit(MBEDTLS_EXIT_FAILURE);
  for (i = 0; i < 16; i++)
    mbedtls_printf("%02x", digest[i]);
  mbedtls_printf("\n\n");
  //mbedtls_exit(MBEDTLS_EXIT_SUCCESS);
}









