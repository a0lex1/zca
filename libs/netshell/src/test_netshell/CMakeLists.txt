file(GLOB_RECURSE test_netshell_SOURCES "./*.cpp" "./*.h")

add_executable (test_netshell ${test_netshell_SOURCES})
target_link_libraries(test_netshell co netshell ${Boost_LIBRARIES})
link_directories(test_netshell STATIC ${Boost_LIBRARY_DIRS})

target_compile_features(test_netshell PRIVATE cxx_std_17)
