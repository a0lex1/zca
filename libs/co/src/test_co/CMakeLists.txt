file(GLOB_RECURSE test_co_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (test_co ${test_co_SOURCES})
target_link_libraries(test_co co ${Boost_LIBRARIES})
link_directories(test_co STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(test_co PRIVATE cxx_std_17)
