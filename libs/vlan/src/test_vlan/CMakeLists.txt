file(GLOB_RECURSE test_vlan_SOURCES "./*.cpp" "./*.h")

add_executable (test_vlan ${test_vlan_SOURCES})
target_link_libraries(test_vlan co proto vlan ${Boost_LIBRARIES})
link_directories(test_vlan STATIC ${Boost_LIBRARY_DIRS})

target_compile_features(test_vlan PRIVATE cxx_std_17)
