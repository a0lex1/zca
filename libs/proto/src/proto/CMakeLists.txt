file(GLOB_RECURSE proto_SOURCES "./*.cpp" "./*.h")

add_library (proto ${proto_SOURCES})
target_compile_features(proto PRIVATE cxx_std_17)
target_link_libraries(proto co)