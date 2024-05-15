#pragma once

#include "./handle.h"

#include "proto/proto_error.h"
#include "proto/proto_message_code.h"

#include "co/base/error.h"

// Tags

enum class VlanErrc {
  ok = 0, // network
  interruped_by_emergency = 7001,
  proto_read_error,
  frame_from_message_failed,
  unknown_proto_message,
  bad_handle,
  connection_refused, // network
  address_already_bound, // network
  switching_off,
  transport_error,
  peer_accept_limit_reached,
  eof, // network
  aborted // network
};

struct VlanErrorInfo {
  VlanErrorInfo() {}
  VlanErrorInfo(const ProtoError& _proto_err) : proto_err(_proto_err) {}
  VlanErrorInfo(ProtoMessageCode _proto_code) : proto_code(_proto_code) {}
  VlanErrorInfo(Errcode _trans_err) : trans_err(_trans_err) {}

  static VlanErrorInfo Fromvlhandle_t(vlhandle_t _vlhandle) {
    VlanErrorInfo ei;
    ei.vlhandle = _vlhandle;
    return ei;
  }

  ProtoError proto_err;
  ProtoMessageCode proto_code;
  vlhandle_t vlhandle{ 0 };
  Errcode trans_err;
};

class VlanError : public co::Error<VlanErrc, VlanErrorInfo> {
public:
  virtual ~VlanError() = default;

  using Error::Error;

  static VlanError NoError() { return VlanError(); }

  // ---------------------------------------------
  Errcode ToErrcodeDefault() const {
    switch (GetErrc()) {
    case VlanErrc::ok: return Errcode();
      //
      // TODO: fill the table
      //
    default: return boost::asio::error::fault;
    }
  }

  const char* GetErrcTitle() const {
    switch (GetErrc()) {
    case VlanErrc::ok: return DefaultErrcTitleOk();
    case VlanErrc::interruped_by_emergency: return "interrupted by emergency";
    case VlanErrc::proto_read_error: return "proto read error";
    case VlanErrc::unknown_proto_message: return "unknown proto message";
    case VlanErrc::bad_handle: return "bad handle";
    case VlanErrc::connection_refused: return "connection refused";
    case VlanErrc::address_already_bound: return "address already bound";
    case VlanErrc::switching_off: return "switching off";
    case VlanErrc::transport_error: return "transport error";
    case VlanErrc::peer_accept_limit_reached: return "peer accept limit reached";
    case VlanErrc::eof: return "eof";
    case VlanErrc::aborted: return "aborted";
    default: return DefaultErrcTitleUnknown();
    }
  }
  std::string MakeErrorMessage() const {
    switch (GetErrc()) {
    case VlanErrc::ok: return DefaultErrcTitleOk();
    case VlanErrc::interruped_by_emergency: return GetErrcTitle();
    case VlanErrc::proto_read_error: return GetErrcTitle() + std::string(" ") + GetErrorInfo().proto_err.MakeErrorMessage();
    case VlanErrc::unknown_proto_message: return GetErrcTitle() + std::string(" ") + co::string_from_int(GetErrorInfo().proto_code);
    case VlanErrc::bad_handle: return GetErrcTitle() + co::string_printf(" %d", GetErrorInfo().vlhandle);
    case VlanErrc::connection_refused: return GetErrcTitle();
    case VlanErrc::address_already_bound: return GetErrcTitle();
    case VlanErrc::switching_off: return GetErrcTitle();
    case VlanErrc::transport_error: {
      std::stringstream ss;
      ss << GetErrcTitle() << " " << GetErrorInfo().trans_err;
      return ss.str();
    }
    case VlanErrc::peer_accept_limit_reached: return GetErrcTitle();
    case VlanErrc::eof: return GetErrcTitle();
    case VlanErrc::aborted: return GetErrcTitle();
    default: return DefaultErrcTitleUnknown();
    }
  }
};