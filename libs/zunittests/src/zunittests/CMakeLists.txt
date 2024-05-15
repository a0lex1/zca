file(GLOB_RECURSE zunittests_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (zunittests ${zunittests_SOURCES})
target_link_libraries(zunittests co proto cc zca ${Boost_LIBRARIES})
link_directories(zunittests STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(zunittests PRIVATE cxx_std_17)
