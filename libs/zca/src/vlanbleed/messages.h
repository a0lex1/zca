#pragma once

#include "./vlan_kernel_config.h"
#include "./emergency_info.h"
#include "./error.h"
#include "./endpoint.h"

#include "proto/proto_message_factory.h"

#include "co/base/bin_writer.h"
#include "co/base/bin_reader.h"

// ---------------------------[ Message Codes ]---------------------------

class vlan_msg_codes {
public:
  static const ProtoMessageCode kVlanConnect = 8001;
  static const ProtoMessageCode kVlanConnectResult = 8002;
  static const ProtoMessageCode kVlanData = 8003;
  static const ProtoMessageCode kVlanDataDrained = 8004;
  static const ProtoMessageCode kVlanQueueDrained = 8005;
  static const ProtoMessageCode kVlanEof = 8006;
  static const ProtoMessageCode kVlanRst = 8007;
  static const ProtoMessageCode kVlanEmergency = 8008;
};

static const char* const VlanMessageTitleFromCode(ProtoMessageCode code) {
  switch (code) {
  case vlan_msg_codes::kVlanConnect: return "VLAN_CONNECT";
  case vlan_msg_codes::kVlanConnectResult: return "VLAN_CONNECT_RESULT";
  case vlan_msg_codes::kVlanData: return "VLAN_DATA";
  case vlan_msg_codes::kVlanDataDrained: return "VLAN_DATA_DRAINED";
  case vlan_msg_codes::kVlanQueueDrained: return "VLAN_QUEUE_DRAINED";
  case vlan_msg_codes::kVlanEof: return "VLAN_EOF";
  case vlan_msg_codes::kVlanRst: return "VLAN_RST";
  case vlan_msg_codes::kVlanEmergency: return "VLAN_EMERGENCY";
  default:
    return "unknown";
  }
}

// ---------------------------[ Message Vector ]---------------------------

static std::vector<ProtoMessageCode> VlanMessageVector = {
  vlan_msg_codes::kVlanConnect, vlan_msg_codes::kVlanConnectResult,
  vlan_msg_codes::kVlanData, vlan_msg_codes::kVlanDataDrained,
  vlan_msg_codes::kVlanQueueDrained, vlan_msg_codes::kVlanEof,
  vlan_msg_codes::kVlanRst, vlan_msg_codes::kVlanEmergency
};

// ---------------------------[ Message Factory ]---------------------------

// friends to all msg classes
class VlanProtoMessageFactory : public ProtoMessageFactory {
public:
  virtual ~VlanProtoMessageFactory() = default;

  Uptr<ProtoMessage> CreateMessageForCode(ProtoMessageCode code) override;
  void CreateTestMessages(std::vector<Uptr<ProtoMessage>>& all_msgs) override;
};

// ---------------------------[ Messages ]---------------------------

class MessageWithKConfigBase {
public:
  void SetAttachedKConfig(Uptr<VlanKernelConfig> kconfig);
  bool HasAttachedKConfig() const;
  const VlanKernelConfig& AttachedKConfig() const;
  VlanKernelConfig& AttachedKConfig();
  bool SerializeKConfigBase(co::BinWriter& writer) const;
  bool UnserializeKConfigBase(co::BinReader& reader);

private:
  Uptr<VlanKernelConfig> kconfig_;
};

class VlanConnectMessage
  :
  public ProtoMessage,
  public MessageWithKConfigBase
{
  static const ProtoMessageCode kCode = vlan_msg_codes::kVlanConnect;

public:
  virtual ~VlanConnectMessage() = default;

  using BinWriter = co::BinWriter;
  using BinReader = co::BinReader;

  VlanConnectMessage(const VlanEndpoint& endpoint,
                     Uptr<VlanKernelConfig> kconfig = nullptr);

  VlanEndpoint GetEndpoint() const { return endpoint_; }
  void SetEndpoint(const VlanEndpoint& endpoint) { endpoint_ = endpoint; }

private:
  VlanEndpoint endpoint_;

private:
  // [ProtoMessage impl]
  bool CompareBody(const ProtoMessage& r) const override;
  bool SerializeBody(BinWriter& writer) const override;
  bool UnserializeBody(BinReader& reader) override;
  static bool WriteVlanEndpoint(BinWriter& writer, const VlanEndpoint& ep);
  static bool ReadVlanEndpoint(BinReader& reader, VlanEndpoint& ep);

private:
  friend class VlanProtoMessageFactory;
  VlanConnectMessage() :ProtoMessage(kCode) {}
};

// ------------------------------------------------------------------------------------------------------------------------------

class VlanConnectResultMessage
  :
  public ProtoMessage,
  public MessageWithKConfigBase
{
  static const ProtoMessageCode kCode = vlan_msg_codes::kVlanConnectResult;

public:
  virtual ~VlanConnectResultMessage() = default;

  using BinWriter = co::BinWriter;
  using BinReader = co::BinReader;

  VlanConnectResultMessage(bool success,
                           Uptr<VlanKernelConfig> kconfig = nullptr);

  bool GetSuccess() const { return success_; }
  void SetSuccess(bool success) { success_ = success; }

private:
  bool success_{ false };

private:
  // [ProtoMessage impl]
  bool CompareBody(const ProtoMessage& r) const override;
  bool SerializeBody(BinWriter& writer) const override;
  bool UnserializeBody(BinReader& reader) override;

private:
  friend class VlanProtoMessageFactory;
  VlanConnectResultMessage() :ProtoMessage(kCode) {}
};

// ------------------------------------------------------------------------------------------------------------------------------

class VlanEmergencyMessage
  :
  public ProtoMessage
{
  static const ProtoMessageCode kCode = vlan_msg_codes::kVlanEmergency;

public:
  virtual ~VlanEmergencyMessage() = default;

  using BinWriter = co::BinWriter;
  using BinReader = co::BinReader;

  VlanEmergencyMessage(const EmergencyInfo& eminfo);

  const EmergencyInfo& GetInfo() const {
    return eminfo_;
  }

private:
  EmergencyInfo eminfo_;

private:
  // [ProtoMessage impl]
  bool CompareBody(const ProtoMessage& r) const override;
  bool SerializeBody(BinWriter& writer) const override;
  bool UnserializeBody(BinReader& reader) override;

private:
  friend class VlanProtoMessageFactory;
  VlanEmergencyMessage() : ProtoMessage(kCode) {}
};

// ------------------------------------------------------------------------------------------------------------------------------

class VlanDataMessage
  :
  public ProtoMessage
{
  static const ProtoMessageCode kCode = vlan_msg_codes::kVlanData;

public:
  virtual ~VlanDataMessage() = default;

  using const_buffers_1 = boost::asio::const_buffers_1;
  using mutable_buffers_1 = boost::asio::mutable_buffers_1;
  using BinWriter = co::BinWriter;
  using BinReader = co::BinReader;

  VlanDataMessage(vlhandle_t vlhandle, Uptr<std::string> ubuf);

  vlhandle_t GetVlHandle() const { return vlhandle_; }

  const_buffers_1 ConstBuffer() const {
    //return boost::asio::buffer_cast<const_buffers_1>(asiobuf_);
    return const_buffers_1(MutableBuffer());
  }
  mutable_buffers_1 MutableBuffer() const {
    return asiobuf_;
  }

private:
  vlhandle_t vlhandle_{ 0 };
  Uptr<std::string> local_buf_;
  mutable_buffers_1 asiobuf_;

private:
  // [ProtoMessage impl]
  bool CompareBody(const ProtoMessage& r) const override;
  bool SerializeBody(BinWriter& writer) const override;
  bool UnserializeBody(BinReader& reader) override;

private:
  friend class VlanProtoMessageFactory;
  VlanDataMessage();
};




