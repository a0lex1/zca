#include "co/base/tests.h"

using namespace co;

void sample_co_async_testkit_testsession(TestInfo& info);
void sample_co_token_buffer(TestInfo& test_info);
void sample_co_xlog(TestInfo& test_info);
void sample_co_xlog_file(TestInfo&);
void sample_co_xlog_ring_file(TestInfo&);
void sample_co_async_repost(TestInfo& test_info);
void sample_co_rape_tcpip(TestInfo&);

co::TestTable co_samples_table = {
  ADD_TEST(sample_co_async_testkit_testsession),
  ADD_TEST(sample_co_token_buffer),
  ADD_TEST(sample_co_xlog),
  ADD_TEST(sample_co_xlog_file),
  ADD_TEST(sample_co_xlog_ring_file),
  ADD_TEST(sample_co_async_repost),
  ADD_TEST(sample_co_rape_tcpip),
};

//uint16_t default_test_port = 31200;
