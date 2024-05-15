file(GLOB_RECURSE netshell_SOURCES "./*.cpp" "./*.h")

add_library (netshell ${netshell_SOURCES})
target_compile_features(netshell PRIVATE cxx_std_17)
target_link_libraries(netshell co)