file(GLOB_RECURSE test_zca_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (test_zca ${test_zca_SOURCES})
target_link_libraries(test_zca co proto cc zca ${Boost_LIBRARIES})
link_directories(test_zca STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(test_zca PRIVATE cxx_std_17)
