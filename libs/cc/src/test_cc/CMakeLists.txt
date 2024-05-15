file(GLOB_RECURSE test_cc_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (test_cc ${test_cc_SOURCES})
target_link_libraries(test_cc co cc proto ${Boost_LIBRARIES})
link_directories(test_cc STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(test_cc PRIVATE cxx_std_17)
