#include "./transport.h"

#include "co/xlog/xlog.h"

using namespace std;

#define llog() \
  syslog(_DBG) << "VlTrans " << std::hex << SPTR(this) << \
    " `" << GET_DEBUG_TAG(DebugTag()) << "` " << std::dec

VlanProtoTransport::VlanProtoTransport(ProtoMessageReader& rdr,
                                       ProtoMessageWriter& wr,
                                       Strand& strand)
  :
  VlanTransport(strand),
  prot_rdr_(rdr), prot_wr_(wr)
{

}

void VlanProtoTransport::Start(RefTracker rt)
{
  DCHECK(frame_handler_);
  GetTSSImpl().BeforeStart(rt);
  ReadNextMessage(RefTracker(CUR_LOC(), [&]() {
    GetTSSImpl().OnIoEnded();
                  },
                  rt));
}

void VlanProtoTransport::StopThreadsafe()
{
  GetTSSImpl().StopThreadsafe();
}

void VlanProtoTransport::StopUnsafe()
{
  prot_rdr_.GetChunkReader().GetStream().Close();
  prot_wr_.GetChunkWriter().GetStream().Close();
}

void VlanProtoTransport::AsyncWriteFrame(const Frame& frame, HandlerWithErrcode handler)
{
  // Called from adapter in single fiber
  DCHECK(InsideStrand());
  DCHECK(!uwrite_handler_);
  uwrite_handler_ = handler;

  prot_wr_.AsyncWriteMessage(frame.GetProtoMessage(),
                             wrap_post(GetStrand(),
                             co::bind(&VlanProtoTransport::HandleWriteMessage,
                             this, _1)));
}

void VlanProtoTransport::ReadNextMessage(RefTracker rt)
{
  prot_rdr_.AsyncReadMessage(cur_proto_msg_,
                             wrap_post(GetStrand(),
                             co::bind(&VlanProtoTransport::HandleReadProtoMessage,
                             this, _1, rt)));
}

void VlanProtoTransport::HandleReadProtoMessage(ProtoError proto_err, RefTracker rt)
{
  DCHECK(InsideStrand());
  DCHECK(frame_handler_);
  if (!proto_err) {
    Frame frame;
    VlanError vlerr;

    frame.FromMessage(move(cur_proto_msg_), vlerr);
    if (!vlerr) {
      // --------------------------------
      // Input the frame, we are in fiber
      // --------------------------------
      auto code = frame.GetProtoMessage().GetCode();
      llog()
        << "inputting frame (" << code << " " << VlanMessageTitleFromCode(code)
        << ") to FrameHandler " << frame_handler_ << "\n";

      frame_handler_->InputFrame(frame);
      // Read next frame
      ReadNextMessage(rt);
      return;
    }
    else {
      last_vlan_error_ = vlerr;
    }
  }
  else {
    llog() << "ReadProtoMessage complete ERROR " << static_cast<int>(proto_err.GetErrc()) << " - " << proto_err.MakeErrorMessage() << "\n";
    last_vlan_error_ = VlanError(VlanErrc::proto_read_error,
                                 VlanErrorInfo(proto_err));

  }
}

void VlanProtoTransport::HandleWriteMessage(Errcode err)
{
  DCHECK(InsideStrand());
  DCHECK(uwrite_handler_);
  auto handler_copy(uwrite_handler_);
  uwrite_handler_ = nullptr;
  // ---------------------------
  // Call user handler
  // ---------------------------
  handler_copy(err);
}

bool VlanProtoTransport::InsideStrand()
{
  return GetTSSImpl().GetStrand().running_in_this_thread();
}

// --------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------

void VlanStreamTransport::Start(RefTracker rt)
{
  prot_trans_.Start(rt);
}

// ----

void VlanStreamTransport::StopThreadsafe()
{
  prot_trans_.StopThreadsafe();
}

void VlanStreamTransport::StopUnsafe()
{
  // no need, stopthreadsafe stops
}
