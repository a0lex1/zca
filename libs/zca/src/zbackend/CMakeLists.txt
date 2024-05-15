file(GLOB_RECURSE zbackend_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (zbackend ${zbackend_SOURCES})
target_link_libraries(zbackend co proto cc zca ${Boost_LIBRARIES})
link_directories(zbackend STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(zbackend PRIVATE cxx_std_17)
