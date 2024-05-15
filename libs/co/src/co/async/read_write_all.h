#pragma once

#include "co/async/stream.h"

namespace co {
namespace async {

//
// |handler| is called from inside |strand|, don't wrap it
//

void AsyncReadAll(Shptr<Strand> strand, StreamIo&, boost::asio::mutable_buffers_1 buf, HandlerWithErrcodeSize handler);
void AsyncWriteAll(Shptr<Strand> strand, StreamIo&, boost::asio::const_buffers_1 buf, HandlerWithErrcodeSize handler);

//
// Without strand, careful! ThreadsafeStopable objects must not use these!
//

void AsyncReadAllNostrand(StreamIo&, boost::asio::mutable_buffers_1 buf, HandlerWithErrcodeSize handler);
void AsyncWriteAllNostrand(StreamIo&, boost::asio::const_buffers_1 buf, HandlerWithErrcodeSize handler);

}}



