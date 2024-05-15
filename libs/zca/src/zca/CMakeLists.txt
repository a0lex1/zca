file(GLOB_RECURSE zca_SOURCES "./*.cpp" "./*.h")

add_library (zca ${zca_SOURCES})
target_compile_features(zca PRIVATE cxx_std_17)
target_link_libraries(zca co proto cc netshell)

