file(GLOB_RECURSE stresstest_zca_cmdexec_SOURCES "./*.cpp" "./*.h")

add_executable (stresstest_zca_cmdexec ${stresstest_zca_cmdexec_SOURCES})
target_link_libraries(stresstest_zca_cmdexec co cc proto zca ${Boost_LIBRARIES})
link_directories(stresstest_zca_cmdexef STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(stresstest_zca_cmdexec PRIVATE cxx_std_17)
