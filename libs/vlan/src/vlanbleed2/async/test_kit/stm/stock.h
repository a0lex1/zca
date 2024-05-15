#pragma once

#include "vlanbleed2/async/test_kit/stm/script_builder.h"
#include <vector>

// namespace stock contains tester scripts
// For use inside co:: and other libraries (which have they own Service impls).

namespace stock {
  using SPair = std::pair<Script, Script>;

  // .Connect() and .Accept() added by default
  // .Close() added by default

  static const std::vector<SPair> gReadwriteScripts{

    SPair({ SBuilder().Read(4).NoErr().S() },
          { SBuilder().Write("test").NoErr().S() }),

    SPair({ SBuilder().Read(5).NoErr().Read(3).NoErr().Read(1).ErrcodeEQ(boost::asio::error::address_family_not_supported).S() },
          { SBuilder().Write("five5threO").NoErr().S() }),

    SPair({ SBuilder().Read(1).NoErr().Write(5).NoErr().Read(27).NoErr().Write(39).NoErr().S() },
          { SBuilder().Write(2).NoErr().Read(4).NoErr().Write(26).NoErr().Read(37).NoErr().Read(1).NoErr().S() }),

   // TODO: read 0 length, ...
  };

  // TODO: Check result without op must dcheck

  static const std::vector<SPair> gShutdownCloseScripts{

    SPair({ SBuilder().ShutdownSend().NoErr().S() },
          { SBuilder().ShutdownSend().NoErr().S() }),

    SPair({ SBuilder().ShutdownSend().NoErr().S() },
          { SBuilder().Read(1).ResultEQ(SOpRes(boost::asio::error::eof, 0)).S() }),

          //<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    SPair({ SBuilder().ShutdownSend().Write(10).ResultEQ(SOpRes(boost::asio::error::shut_down)).S() },
          { SBuilder().Read(1).ResultEQ(SOpRes(boost::asio::error::eof)).Close().S() }),

    SPair({ SBuilder().ShutdownSend().Write(10).ResultEQ(SOpRes(boost::asio::error::shut_down)).S() },
          { SBuilder().Read(1).ResultEQ(SOpRes(boost::asio::error::eof)).Close().Read(10).ResultEQ(SOpRes(boost::asio::error::bad_descriptor)).S() }),

    SPair({ SBuilder().Write("t").ShutdownSend().NoErr().S() },
          { SBuilder().Read(1).ResultEQ(SOpRes(boost::asio::error::eof, 1)).S() }),

    SPair({ SBuilder().Write("xxxty").ShutdownSend().NoErr().S() },
          { SBuilder().Read(100).ResultEQ(SOpRes(boost::asio::error::eof, 6, "xxxty")).S() }),

    // TODO: send, close, shutdown, etc xxx
  };

}

