if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.23)
    project(LANBootstrapContract LANGUAGES CXX)
    enable_testing()
    set(OPENTS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
else()
    set(OPENTS_ROOT "${CMAKE_SOURCE_DIR}")
endif()

add_executable(LANBootstrapContract
    "${CMAKE_CURRENT_SOURCE_DIR}/lanbootstrap.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/connectionstubs.cpp"
    "${OPENTS_ROOT}/code/landiagnostics.cpp"
    "${OPENTS_ROOT}/code/lanbootstrap.cpp"
    "${OPENTS_ROOT}/code/ipxaddr.cpp"
    "${OPENTS_ROOT}/code/netsocket_null.cpp"
    "${OPENTS_ROOT}/code/netadmit.cpp"
    "${OPENTS_ROOT}/code/wsproto.cpp"
    "${OPENTS_ROOT}/code/wspudp.cpp"
    "${OPENTS_ROOT}/code/combuf.cpp"
    "${OPENTS_ROOT}/code/connect.cpp"
    "${OPENTS_ROOT}/code/ipxconn.cpp"
    "${OPENTS_ROOT}/code/ipxgconn.cpp"
    "${OPENTS_ROOT}/code/nettime.cpp"
    "${OPENTS_ROOT}/code/nettiming.cpp"
)
if(WIN32)
    target_sources(LANBootstrapContract PRIVATE "${OPENTS_ROOT}/code/netsocket_win32.cpp")
else()
    target_sources(LANBootstrapContract PRIVATE "${OPENTS_ROOT}/code/netsocket_posix.cpp")
endif()

target_compile_features(LANBootstrapContract PRIVATE cxx_std_20)
target_include_directories(LANBootstrapContract PRIVATE "${OPENTS_ROOT}/code")
if(TARGET OpenTSBuildStamp)
    target_include_directories(LANBootstrapContract PRIVATE "${OPENTS_GENERATED_DIR}")
    add_dependencies(LANBootstrapContract OpenTSBuildStamp)
endif()
target_compile_definitions(LANBootstrapContract PRIVATE NOMINMAX)
if(MSVC)
    target_compile_options(LANBootstrapContract PRIVATE
        $<$<CONFIG:Debug>:/MTd /EHsc /Zc:__cplusplus>
        $<$<CONFIG:Release>:/MT /EHsc /Zc:__cplusplus>
    )
    target_link_libraries(LANBootstrapContract PRIVATE kernel32 user32 shell32 ws2_32 iphlpapi winmm)
endif()
set_target_properties(LANBootstrapContract PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test-bin")
add_test(NAME lanbootstrap-contract COMMAND LANBootstrapContract)
