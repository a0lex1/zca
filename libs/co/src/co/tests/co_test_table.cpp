#include "co/base/tests.h"

using namespace co;

void test_co_filter_chain(TestInfo& ti);
void test_co_filter_chain_to_chain(TestInfo& ti);
void test_co_filter_chain_two_bases(TestInfo& ti);
void test_co_reftracker(TestInfo&);
void test_co_reftracker_context(TestInfo& test_info);
void test_co_net_distance(TestInfo&);
void test_co_net_range_iterator(TestInfo&);
void test_co_net_subrange_enumerator(TestInfo&);
void test_co_net_address_calc(TestInfo&);
void test_co_xlog_c99_(TestInfo&);
void test_co_xlog_newlines(TestInfo&);
void test_co_xlog(TestInfo&);
void test_co_xlog_configs(TestInfo&);
void test_co_xlog_printsinks(TestInfo&);
void test_co_parse_cmdline_to_argv(TestInfo&);
void test_co_strings(TestInfo&);
void test_co_xlog_CLoggerOstreamRingDevice(TestInfo&);
void test_co_keyed_cmdline(TestInfo&);
void test_co_keyed_cmdline_textualize(TestInfo& test_info);
void test_co_cmdline_section_split(TestInfo& ti);
void test_co_config(TestInfo&);
void test_co_process_io_redirector(TestInfo&);
void test_co_base64(TestInfo&);

void test_co_async_sesslist_server(TestInfo&);
void test_co_async_sesslist_server_instantstop(TestInfo&);
void test_co_async_sesslist_server_instantstop_multiclient(TestInfo&);

void test_co_async_capsule_normal_run(TestInfo&);
void test_co_async_capsule_unrecov_preparephase(TestInfo&);
void test_co_async_capsule_recov_preparephase(TestInfo&);
void test_co_async_capsule_unrecov_startphase(TestInfo&);
void test_co_async_capsule_recov_startphase(TestInfo&);
void test_co_async_capsule_unrecov_runphase(TestInfo&);
void test_co_async_capsule_recov_runphase(TestInfo&);
void test_co_async_capsule_unhandled_excp(TestInfo&);

void test_co_bin_reader_writer(TestInfo&);
void test_co_async_coro0(TestInfo&);
void test_co_async_coro1(TestInfo&);
void test_co_async_coro2(TestInfo&);
void test_co_tokenizers_match_edgecases(TestInfo&);
void test_co_bin_reader_writer_corrupt(TestInfo& test_info);
void test_co_print_boost_diagnostic_information(TestInfo&);
void test_co_async_event(TestInfo&);
void test_co_async_event_wtimer(TestInfo&);
void test_co_async_sync_event_wtimer(TestInfo&);
void test_co_string_from_hex(TestInfo&);
void test_co_string_radix16(TestInfo&);
void test_co_cmdline_parsed_command_line(TestInfo&);
void test_co_cmdline_parsed_command_line_empty(TestInfo&);
void test_co_textualize_configs(TestInfo& ti);
void test_co_rand_gen(TestInfo&);
void test_co_async_abortedstop(TestInfo&);

co::TestTable co_test_table = {
  ADD_TEST(test_co_filter_chain),
  ADD_TEST(test_co_filter_chain_to_chain),
  ADD_TEST(test_co_filter_chain_two_bases),
  ADD_TEST(test_co_reftracker),
  ADD_TEST(test_co_reftracker_context),
  ADD_TEST(test_co_net_distance),
  ADD_TEST(test_co_net_range_iterator),
  ADD_TEST(test_co_net_subrange_enumerator),
  ADD_TEST(test_co_net_address_calc),
  ADD_TEST(test_co_xlog_c99_),
  ADD_TEST(test_co_xlog_newlines),
  ADD_TEST(test_co_xlog),
  ADD_TEST(test_co_xlog_configs),
  ADD_TEST(test_co_xlog_printsinks),
  ADD_TEST(test_co_parse_cmdline_to_argv),
  ADD_TEST(test_co_strings),
  ADD_TEST(test_co_xlog_CLoggerOstreamRingDevice),
  ADD_TEST(test_co_keyed_cmdline),
  ADD_TEST(test_co_keyed_cmdline_textualize),
  ADD_TEST(test_co_cmdline_section_split),
  ADD_TEST(test_co_config),
  ADD_TEST(test_co_process_io_redirector),
  ADD_TEST(test_co_base64),

  ADD_TEST(test_co_async_sesslist_server),
  ADD_TEST(test_co_async_sesslist_server_instantstop),
  ADD_TEST(test_co_async_sesslist_server_instantstop_multiclient),

  ADD_TEST(test_co_async_capsule_normal_run),
  ADD_TEST(test_co_async_capsule_unrecov_preparephase),
  ADD_TEST(test_co_async_capsule_recov_preparephase),
  ADD_TEST(test_co_async_capsule_unrecov_startphase),
  ADD_TEST(test_co_async_capsule_recov_startphase),
  ADD_TEST(test_co_async_capsule_unrecov_runphase),
  ADD_TEST(test_co_async_capsule_recov_runphase),
  ADD_TEST(test_co_async_capsule_unhandled_excp),

  ADD_TEST(test_co_bin_reader_writer),
  ADD_TEST(test_co_async_coro0),
  ADD_TEST(test_co_async_coro1),
  ADD_TEST(test_co_async_coro2),
  ADD_TEST(test_co_tokenizers_match_edgecases),
  ADD_TEST(test_co_bin_reader_writer_corrupt),
  ADD_TEST(test_co_print_boost_diagnostic_information),
  ADD_TEST(test_co_async_event),
  ADD_TEST(test_co_async_event_wtimer),
  ADD_TEST(test_co_async_sync_event_wtimer),
  ADD_TEST(test_co_string_from_hex),
  ADD_TEST(test_co_string_radix16),
  ADD_TEST(test_co_cmdline_parsed_command_line),
  ADD_TEST(test_co_cmdline_parsed_command_line_empty),
  ADD_TEST(test_co_textualize_configs),
  ADD_TEST(test_co_rand_gen),
  ADD_TEST(test_co_async_abortedstop)
};

