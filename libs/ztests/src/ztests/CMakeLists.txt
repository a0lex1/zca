file(GLOB_RECURSE ztests_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (ztests ${ztests_SOURCES})
target_link_libraries(ztests co proto cc zca ${Boost_LIBRARIES})
link_directories(ztests STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(ztests PRIVATE cxx_std_17)
