file(GLOB_RECURSE zbackfront_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (zbackfront ${zbackfront_SOURCES})
target_link_libraries(zbackfront co proto cc zca ${Boost_LIBRARIES})
link_directories(zbackfront STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(zbackfront PRIVATE cxx_std_17)
