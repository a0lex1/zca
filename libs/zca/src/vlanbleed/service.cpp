#include "./service.h"
#include "./factories.h"

using namespace co;
using namespace co::async;

VlanService::VlanService(io_context& ioc, VlanNativeApi& native_api)
  : native_api_(native_api), ioc_(ioc)
{

}

Uptr<StreamFactory> VlanService::CreateStreamFactory() {
  // Streams don't need adap_st_ on creation, they're connected later
  return make_unique<StreamFactoryImpl>(ioc_);
}

Uptr<StreamAcceptorFactory> VlanService::CreateStreamAcceptorFactory() {
  return make_unique<StreamAcceptorFactoryImpl>(ioc_, native_api_);
}

Uptr<StreamConnectorFactory> VlanService::CreateStreamConnectorFactory() {
  return make_unique<StreamConnectorFactoryImpl>(native_api_);
}
