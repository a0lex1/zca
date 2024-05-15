file(GLOB_RECURSE zagent_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (zagent ${zagent_SOURCES})
target_link_libraries(zagent co proto cc zca ${Boost_LIBRARIES})
link_directories(zagent STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(zagent PRIVATE cxx_std_17)
