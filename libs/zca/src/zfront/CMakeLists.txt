file(GLOB_RECURSE zfront_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (zfront ${zfront_SOURCES})
target_link_libraries(zfront co proto cc zca ${Boost_LIBRARIES})
link_directories(zfront STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(zfront PRIVATE cxx_std_17)
