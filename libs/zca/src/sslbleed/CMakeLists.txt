file(GLOB_RECURSE sslbleed_SOURCES "./*.c" "./*.cpp" "./*.h" "./*.hpp")

add_executable (sslbleed ${sslbleed_SOURCES})
target_link_libraries(sslbleed co proto cc zca ${Boost_LIBRARIES} MbedTLS::mbedcrypto MbedTLS::mbedtls MbedTLS::mbedx509)
link_directories(sslbleed STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(sslbleed PRIVATE cxx_std_17)

