file(GLOB_RECURSE samples_zca_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (samples_zca ${samples_zca_SOURCES})
target_link_libraries(samples_zca co proto cc zca ${Boost_LIBRARIES})
link_directories(samples_zca STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(samples_zca PRIVATE cxx_std_17)
