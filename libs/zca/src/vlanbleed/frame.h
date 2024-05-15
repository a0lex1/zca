#pragma once

#include "./vlanbleed/messages.h"
#include "./vlanbleed/error.h"

#include "proto/proto_message.h"

#include "co/common.h"

#include <algorithm>

// Frames naturally works on ProtoMessage-s. ProtoMessage is a good
// concept to store such kind of frame.
// Frame is a container for proto msg that does verification.
//
class Frame {
public:
  //virtual ~Frame() = default;

  Frame() {}
  Frame(Uptr<ProtoMessage> pmsg) : pmsg_(std::move(pmsg)) {}

  const ProtoMessage& GetProtoMessage() const { return *pmsg_.get(); }
  ProtoMessage& GetProtoMessage() { return const_cast<ProtoMessage&>(static_cast<const Frame&>(*this).GetProtoMessage()); } // like this bdsm?

  // move (faster)
  void FromMessage(Uptr<ProtoMessage> pmsg, VlanError& vlerr) {
    VerifyMessage(*pmsg.get(), vlerr);
    if (!vlerr) {
      pmsg_ = std::move(pmsg);
    }
  }
  // copy
/*
  void FromMessage(const ProtoMessage& pmsg, VlanError& vlerr) {
    VerifyMessage(pmsg, vlerr);
    if (!vlerr) {
      pmsg_ = pmsg.Clone();
    }
  }
*/

  const VlanConnectMessage& AsConnect() const {
    DCHECK(pmsg_->GetCode() == vlan_msg_codes::kVlanConnect);
    return static_cast<VlanConnectMessage&>(*pmsg_.get());
  }
  VlanConnectMessage& AsConnect() {
    return const_cast<VlanConnectMessage&>(
      static_cast<const Frame*>(this)->AsConnect());
  }

  const VlanConnectResultMessage& AsConnectResult() const {
    DCHECK(pmsg_->GetCode() == vlan_msg_codes::kVlanConnectResult);
    return static_cast<VlanConnectResultMessage&>(*pmsg_.get());
  }
  VlanConnectResultMessage& AsConnectResult() {
    return const_cast<VlanConnectResultMessage&>(
      static_cast<const Frame*>(this)->AsConnectResult());
  }

  const VlanEmergencyMessage& AsEmergency() const {
    DCHECK(pmsg_->GetCode() == vlan_msg_codes::kVlanEmergency);
    return static_cast<VlanEmergencyMessage&>(*pmsg_.get());
  }
  VlanEmergencyMessage& AsEmergency() {
    return const_cast<VlanEmergencyMessage&>(
      static_cast<const Frame*>(this)->AsEmergency());
  }

  const VlanDataMessage& AsData() const {
    DCHECK(pmsg_->GetCode() == vlan_msg_codes::kVlanData);
    return static_cast<VlanDataMessage&>(*pmsg_.get());
  }
  VlanDataMessage& AsData() {
    return const_cast<VlanDataMessage&>(
      static_cast<const Frame*>(this)->AsData());
  }

private:
  void VerifyMessage(const ProtoMessage& pmsg, VlanError& vlerr) {
    const std::vector<ProtoMessageCode>& allowed_msgs = VlanMessageVector;
    if (std::find(allowed_msgs.begin(), allowed_msgs.end(), pmsg.GetCode())
        == allowed_msgs.end())
    {
      vlerr = VlanError(VlanErrc::unknown_proto_message,
                        VlanErrorInfo(pmsg.GetCode()));
      return;
    }
    // Other verification
  }

private:
  Uptr<ProtoMessage> pmsg_;
};

class FrameWriterST {
public:
  virtual ~FrameWriterST() = default;

  // called in single fiber, no need to synchronize at all
  virtual void AsyncWriteFrame(const Frame&, HandlerWithErrcode) = 0;
};

class FrameHandler {
public:
  virtual ~FrameHandler() = default;

  // can be called without waiting
  virtual void InputFrame(const Frame&) = 0;
};

