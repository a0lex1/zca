#include "./messages.h"

using namespace co;
using namespace std;

Uptr<ProtoMessage> VlanProtoMessageFactory::CreateMessageForCode(
  ProtoMessageCode code)
{
  switch (code) {
  case vlan_msg_codes::kVlanConnect:
    return Uptr<VlanConnectMessage>(new VlanConnectMessage());
  case vlan_msg_codes::kVlanConnectResult:
    return Uptr<VlanConnectResultMessage>(new VlanConnectResultMessage());
  case vlan_msg_codes::kVlanData:
    return Uptr<VlanDataMessage>(new VlanDataMessage());
  case vlan_msg_codes::kVlanEmergency:
    return Uptr<VlanEmergencyMessage>(new VlanEmergencyMessage());
  default:
    return nullptr;
  }
}

void VlanProtoMessageFactory::CreateTestMessages(
  std::vector<Uptr<ProtoMessage>>& all_msgs)
{
  static const char tmp[5] = "";

  all_msgs.emplace_back(make_unique<VlanConnectMessage>(VlanEndpoint::Loopback(),
                        Uptr<VlanKernelConfig>(new VlanKernelConfig{ 19, 5,10 })));

  all_msgs.emplace_back(make_unique<VlanConnectResultMessage>(true,
                        Uptr<VlanKernelConfig>(new VlanKernelConfig{ 10, 13, 17 })));

  all_msgs.emplace_back(make_unique<VlanDataMessage>((vlhandle_t)89312,
                        make_unique<string>("sex")));
  //all_msgs.emplace_back(make_unique<VlanDataDrainedMessage>());
  //all_msgs.emplace_back(make_unique<VlanQueueDrainedMessage>());
  //all_msgs.emplace_back(make_unique<VlanEofMessage>());
  //all_msgs.emplace_back(make_unique<VlanRstMessage>());
  all_msgs.emplace_back(make_unique<VlanEmergencyMessage>(EmergencyInfo(
  )));
}

// --------------------------

VlanConnectMessage::VlanConnectMessage(const VlanEndpoint& endpoint,
                                       Uptr<VlanKernelConfig> kconfig/*=nullptr*/)
  :
  ProtoMessage(kCode),
  endpoint_(endpoint)
{
  if (kconfig != nullptr) {
    SetAttachedKConfig(move(kconfig));
  }
}

bool VlanConnectMessage::CompareBody(const ProtoMessage& r) const
{
  const auto& rr = static_cast<const VlanConnectMessage&>(r);
  return
    this->endpoint_ == rr.endpoint_ &&
    this->AttachedKConfig() == rr.AttachedKConfig();
}

bool VlanConnectMessage::SerializeBody(BinWriter& writer) const
{
  if (!WriteVlanEndpoint(writer, endpoint_)) {
    return false;
  }
  if (!SerializeKConfigBase(writer)) {
    return false;
  }
  return true;
}

bool VlanConnectMessage::UnserializeBody(BinReader& reader)
{
  VlanEndpoint ep;
  if (!ReadVlanEndpoint(reader, ep)) {
    return false;
  }
  if (!UnserializeKConfigBase(reader)) {
    return false;
  }
  endpoint_ = ep;
  return true;
}

bool VlanConnectMessage::WriteVlanEndpoint(BinWriter& writer, const VlanEndpoint& ep)
{
  if (!writer.WriteUint32(ep.GetAddress().Host())) {
    return false;
  }
  if (!writer.WriteUint32(ep.GetPort())) {
    return false;
  }
  return true;
}

bool VlanConnectMessage::ReadVlanEndpoint(BinReader& reader, VlanEndpoint& ep)
{
  uint32_t vlhost;
  if (!reader.ReadUint32(vlhost)) {
    return false;
  }
  uint32_t vlport;
  if (!reader.ReadUint32(vlport)) {
    return false;
  }
  ep = VlanEndpoint(VlanAddress(vlhost), vlport);
  return true;
}

// ---------------------------------------------------------------------------------

bool VlanEmergencyMessage::CompareBody(const ProtoMessage& r) const
{
  const auto& rr = static_cast<const VlanEmergencyMessage&>(r);
  return this->eminfo_ == rr.eminfo_;
}

bool VlanEmergencyMessage::SerializeBody(BinWriter& writer) const
{
  if (!writer.WriteUint32(static_cast<uint32_t>(eminfo_.reason))) {
    return false;
  }
  if (!writer.WriteUint16(eminfo_.violation_tag)) {
    return false;
  }
  if (!writer.WriteInt32(eminfo_.writeframe_err)) {
    return false;
  }
  return true;
}

bool VlanEmergencyMessage::UnserializeBody(BinReader& reader)
{
  uint32_t v;
  if (!reader.ReadUint32(v)) {
    return false;
  }
  ViolationTag viol_tag;
  if (!reader.ReadUint16(viol_tag)) {
    return false;
  }
  int wferrno;
  if (!reader.ReadInt32(wferrno)) {
    return false;
  }
  EmergencyInfo ei{
    static_cast<EmergencyReason>(v),
    viol_tag, wferrno
  };
  *this = VlanEmergencyMessage(ei);
  return true;
}

VlanEmergencyMessage::VlanEmergencyMessage(const EmergencyInfo& eminfo)
  : ProtoMessage(kCode), eminfo_(eminfo)
{

}

// ---------------------------------------------------------------------------------

VlanConnectResultMessage::VlanConnectResultMessage(bool success,
                                                   Uptr<VlanKernelConfig> kconfig /*=nullptr*/)
  :
  ProtoMessage(kCode),
  success_(success)
{
  if (kconfig) {
    SetAttachedKConfig(move(kconfig));
  }
}

bool VlanConnectResultMessage::CompareBody(const ProtoMessage& r) const
{
  const auto& rr = static_cast<const VlanConnectResultMessage&>(r);
  return
    this->success_ == rr.success_ &&
    this->AttachedKConfig() == rr.AttachedKConfig();
}

bool VlanConnectResultMessage::SerializeBody(BinWriter& writer) const
{
  if (!SerializeKConfigBase(writer)) {
    return false;
  }
  if (!writer.WriteUint8(success_ ? 1 : 0)) {
    return false;
  }
  return true;
}

bool VlanConnectResultMessage::UnserializeBody(BinReader& reader)
{
  if (!UnserializeKConfigBase(reader)) {
    return false;
  }
  uint8_t scs;
  if (!reader.ReadUint8(scs)) {
    return false;
  }
  if (scs != 0 && scs != 1) {
    // bad value
    return false;
  }
  success_ = scs == 0 ? false : true;
  return true;
}

// -------------------------------------------------------------------------------------------------------------------------------------

namespace {
  static char* kNothing = "";
}

VlanDataMessage::VlanDataMessage(vlhandle_t vlhandle, Uptr<string> ubuf)
  :
  ProtoMessage(kCode),
  vlhandle_(vlhandle),
  local_buf_(move(ubuf)),
  asiobuf_(&local_buf_->at(0), local_buf_->length())
{
}

VlanDataMessage::VlanDataMessage()
  : ProtoMessage(kCode), vlhandle_(0), asiobuf_(&kNothing, 0)
{

}

bool VlanDataMessage::CompareBody(const ProtoMessage& r) const
{
  const auto& rr = static_cast<const VlanDataMessage&>(r);
  auto const as(asiobuf_.size());
  auto const bs(rr.asiobuf_.size());
  return vlhandle_ == rr.vlhandle_ &&
    as == bs &&
    !memcmp(asiobuf_.data(), rr.asiobuf_.data(), as);
}

bool VlanDataMessage::SerializeBody(BinWriter& writer) const
{
  static_assert(sizeof(vlhandle_) == 1);
  if (!writer.WriteUint8(vlhandle_)) {
    return false;
  }
  if (!writer.WriteByteArray(asiobuf_.data(), asiobuf_.size())) {
    return false;
  }
  return true;
}

bool VlanDataMessage::UnserializeBody(BinReader& reader)
{
  static_assert(sizeof(vlhandle_) == 1); // how you like this, Elon?

  if (!reader.ReadUint8(vlhandle_)) {
    return false;
  }
  const uint8_t* bptr;
  uint32_t bpsize;
  if (!reader.ReadByteArray(bptr, &bpsize)) {
    return false;
  }
  local_buf_ = make_unique<std::string>((const char*)(bptr), static_cast<size_t>(bpsize));
  asiobuf_ = mutable_buffers_1((char*)local_buf_->c_str(), local_buf_->size());
  return true;
}

void MessageWithKConfigBase::SetAttachedKConfig(Uptr<VlanKernelConfig> kconfig)
{
  kconfig_ = std::move(kconfig);
}

bool MessageWithKConfigBase::HasAttachedKConfig() const
{
  return kconfig_ != nullptr;
}

const VlanKernelConfig& MessageWithKConfigBase::AttachedKConfig() const
{
  DCHECK(kconfig_);
  return *kconfig_.get();
}

VlanKernelConfig& MessageWithKConfigBase::AttachedKConfig()
{
  return
    const_cast<VlanKernelConfig&>(static_cast<const MessageWithKConfigBase*>(this)->AttachedKConfig());
}

bool MessageWithKConfigBase::SerializeKConfigBase(co::BinWriter& writer) const
{
  // bool HAS or NOT
  if (!writer.WriteUint8(HasAttachedKConfig() ? 1 : 0)) {
    return false;
  }
  if (HasAttachedKConfig()) {
    if (!AttachedKConfig().Serialize(writer)) {
      return false;
    }
  }
  return true;
}

bool MessageWithKConfigBase::UnserializeKConfigBase(co::BinReader& reader)
{
  uint8_t has_kcfg;
  if (!reader.ReadUint8(has_kcfg)) {
    return false;
  }
  if (has_kcfg != 0) {
    if (has_kcfg == 1) {
      // Try unserialize
      SetAttachedKConfig(make_unique<VlanKernelConfig>());
      DCHECK(HasAttachedKConfig());
      if (!AttachedKConfig().Unserialize(reader)) {
        return false;
      }
      // ok, unserialized
    }
    else {
      // has_kcfg is bool , must be 1/0
      return false;
    }
  }
  return true;
}
