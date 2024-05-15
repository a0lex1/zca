file(GLOB_RECURSE stresstest_co_listserver_SOURCES "./*.cpp" "./*.h")

add_executable (stresstest_co_listserver ${stresstest_co_listserver_SOURCES})
target_link_libraries(stresstest_co_listserver co ${Boost_LIBRARIES})
link_directories(stresstest_co_listserver STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(stresstest_co_listserver PRIVATE cxx_std_17)
