file(GLOB_RECURSE testproject_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (testproject ${testproject_SOURCES})
target_link_libraries(testproject co proto cc zca ${Boost_LIBRARIES})
link_directories(testproject STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(testproject PRIVATE cxx_std_17)
