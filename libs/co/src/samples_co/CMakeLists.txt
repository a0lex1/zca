file(GLOB_RECURSE samples_co_SOURCES "./*.cpp" "./*.h")

add_executable (samples_co ${samples_co_SOURCES})
target_link_libraries(samples_co co ${Boost_LIBRARIES})
link_directories(samples_co STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(samples_co PRIVATE cxx_std_17)
