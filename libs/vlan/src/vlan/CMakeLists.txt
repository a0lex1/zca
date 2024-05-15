file(GLOB_RECURSE vlan_SOURCES "./*.cpp" "./*.h")

add_library (vlan ${vlan_SOURCES})
target_compile_features(vlan PRIVATE cxx_std_17)
target_link_libraries(vlan co proto)