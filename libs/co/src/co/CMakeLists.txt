file(GLOB_RECURSE co_SOURCES "./*.c" "./*.cpp" "./*.h")

add_library (co ${co_SOURCES})
target_compile_features(co PRIVATE cxx_std_17)

