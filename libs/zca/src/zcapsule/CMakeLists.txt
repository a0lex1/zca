file(GLOB_RECURSE zcapsule_SOURCES "./*.c" "./*.cpp" "./*.h")

add_executable (zcapsule ${zcapsule_SOURCES})
target_link_libraries(zcapsule co proto cc zca ${Boost_LIBRARIES})
link_directories(zcapsule STATIC ${Boost_LIBRARY_DIRS})
target_compile_features(zcapsule PRIVATE cxx_std_17)
