file(GLOB_RECURSE test_proto_SOURCES "./*.cpp" "./*.h")

add_executable (test_proto ${test_proto_SOURCES})
target_link_libraries(test_proto co proto ${Boost_LIBRARIES})
link_directories(test_proto STATIC ${Boost_LIBRARY_DIRS})

target_compile_features(test_proto PRIVATE cxx_std_17)
