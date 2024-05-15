file(GLOB_RECURSE vlanbleed2_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (vlanbleed2 ${vlanbleed2_SOURCES})
target_link_libraries(vlanbleed2 co proto ${Boost_LIBRARIES})
link_directories(vlanbleed2 STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(vlanbleed2 PRIVATE cxx_std_17)
