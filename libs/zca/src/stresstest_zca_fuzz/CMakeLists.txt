file(GLOB_RECURSE stresstest_zca_fuzz_SOURCES "./*.cpp" "./*.h")

add_executable (stresstest_zca_fuzz ${stresstest_zca_fuzz_SOURCES})
target_link_libraries(stresstest_zca_fuzz co cc proto zca ${Boost_LIBRARIES})
link_directories(stresstest_zca_cmdexef STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(stresstest_zca_fuzz PRIVATE cxx_std_17)
