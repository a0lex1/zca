file(GLOB_RECURSE stresstest_co_taskmgr_SOURCES "./*.cpp" "./*.h")

add_executable (stresstest_co_taskmgr ${stresstest_co_taskmgr_SOURCES})
target_link_libraries(stresstest_co_taskmgr co ${Boost_LIBRARIES})
link_directories(stresstest_co_taskmgr STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(stresstest_co_taskmgr PRIVATE cxx_std_17)
