cmake_minimum_required (VERSION 3.15)

project ("z")

#add_definitions(-DBOOST_ASIO_ENABLE_HANDLER_TRACKING)

# doesn't help against gcc warnings
#set(CMAKE_CXX_FLAGS "-Wmultichar")

set(Boost_USE_MULTITHREADED ON)

IF (CMAKE_BUILD_TYPE STREQUAL "Release")
  # Release
  add_compile_options(-DCO_XLOG_DISABLE)
  add_compile_options(-DCO_SILENCE_DCHECKS)

  #add_compile_options(-DCO_DISABLE_LOCATION)
  add_compile_options(-DCO_RT_TRACKING_REFTRACKERS)
  add_compile_options(-DCO_RT_TRACKING_RTCONTEXTS)
ELSE()
  # Debug and RelWithDebInfo
  add_compile_options(-DCO_RT_TRACKING_REFTRACKERS)
  add_compile_options(-DCO_RT_TRACKING_RTCONTEXTS)
ENDIF()

IF (WIN32)
  # Windows
  MESSAGE(CHECK_START "[ Windows ]")
  add_definitions(/D_WIN32_WINNT=0x0601) # Win7
  #set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME OFF)
ELSE()
  # Linux/Mac
  #add_compile_options(-Werror=stringop-overflow) #mbedTls require it < gcc says its unknown
  add_compile_options(-Werror=shift-overflow)
  add_compile_options(-gdwarf-4) # otherwise valgrind says ### unhandled dwarf2 abbrev form code 0x25  ... etc.
  #add_compile_options(-fsanitize=address)
  #add_link_options(-fsanitize=address)
  IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
      # Linux and MAC, Debug
      MESSAGE(CHECK_START "[ Linux/Mac, Debug ]")
      add_compile_options(-finstrument-functions)
  ELSEIF(CMAKE_BUILD_TYPE STREQUAL "Release")
      # Linux and Mac, Release
      MESSAGE(CHECK_START "[ Linux/Mac, Release ]")
      add_compile_options(-O3)
  ELSE()
      MESSAGE(CHECK_START "[ Linux/Mac, Release-based ]")
      add_compile_options(-O3)
  ENDIF()
ENDIF()


set(CMAKE_POSITION_INDEPENDENT_CODE ON)

#SET(BUILD_SHARED_LIBRARIES OFF)


# Boost
find_package(Boost 1.74.0 REQUIRED COMPONENTS date_time filesystem system thread context coroutine)
set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
include_directories(${Boost_INCLUDE_DIRS})


# MbedTLS
#include(./libs/mbedtls/build/cmake/MbedTLSConfig.cmake)
#find_package(MbedTLS)

#add_subdirectory(./libs/mbedtls/)
################

include_directories(libs/co/src/)
include_directories(libs/proto/src/)
include_directories(libs/netshell/src/)
###include_directories(libs/vlan/src/)
include_directories(libs/cc/src/)
include_directories(libs/zca/src/)

###add_subdirectory(libs/tabulate/)
include_directories(libs/tabulate/include)

add_subdirectory(libs/co/src/co/)
add_subdirectory(libs/proto/src/proto/)
add_subdirectory(libs/netshell/src/netshell/)
###add_subdirectory(libs/vlan/src/vlan/)
add_subdirectory(libs/cc/src/cc/)
add_subdirectory(libs/zca/src/zca/)

#add_subdirectory(libs/co/src/test_co/)
#add_subdirectory(libs/co/src/stresstest_co_listserver/)
#add_subdirectory(libs/co/src/stresstest_co_taskmgr/)
#add_subdirectory(libs/co/src/samples_co/)
#add_subdirectory(libs/proto/src/test_proto/)
add_subdirectory(libs/netshell/src/test_netshell/)
###add_subdirectory(libs/vlan/src/test_vlan/)
#add_subdirectory(libs/cc/src/test_cc/)
#add_subdirectory(libs/zca/src/test_zca/)
#add_subdirectory(libs/zca/src/stresstest_zca_cmdexec/)
###add_subdirectory(libs/zca/src/stresstest_zca_fuzz/)
#add_subdirectory(libs/zca/src/samples_zca/)
#add_subdirectory(libs/zca/src/testproject/)

###add_subdirectory(libs/vlan/src/vlanbleed2/)
###add_subdirectory(libs/zca/src/vlanbleed/) # old
###add_subdirectory(libs/zca/src/sslbleed/)

add_subdirectory(libs/zca/src/zagent/)
add_subdirectory(libs/zca/src/zbackend/)
add_subdirectory(libs/zca/src/zcapsule/)
add_subdirectory(libs/zca/src/zbackfront/)
add_subdirectory(libs/zca/src/zfront/)

add_subdirectory(libs/ztests/src/ztests/)
#add_subdirectory(libs/zunittests/src/zunittests/)

