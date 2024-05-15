file(GLOB_RECURSE vlanbleed_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (vlanbleed ${vlanbleed_SOURCES})
target_link_libraries(vlanbleed co proto cc zca ${Boost_LIBRARIES})
link_directories(vlanbleed STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(vlanbleed PRIVATE cxx_std_17)
