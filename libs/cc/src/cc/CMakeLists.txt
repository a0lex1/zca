file(GLOB_RECURSE cc_SOURCES "./*.cpp" "./*.h")

add_library (cc ${cc_SOURCES})
target_link_libraries(cc co proto)

target_compile_features(cc PRIVATE cxx_std_17)
