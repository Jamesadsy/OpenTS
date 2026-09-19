add_executable(OpenTSDonorHostSmoke
    hostsmoke.cpp
    ${CMAKE_SOURCE_DIR}/code/bgfxbackend.cpp
)

target_compile_features(OpenTSDonorHostSmoke PRIVATE cxx_std_20)
target_include_directories(OpenTSDonorHostSmoke PRIVATE
    ${CMAKE_SOURCE_DIR}/code
    ${OPENTS_GENERATED_DIR}
    ${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bgfx/include
    ${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bx/include
    ${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bimg/include
    ${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bgfx/examples/common/imgui
)
target_compile_definitions(OpenTSDonorHostSmoke PRIVATE ${OPENTS_BX_CONFIG_DEBUG})
target_link_libraries(OpenTSDonorHostSmoke PRIVATE
    win32compat
    bgfx
    bx
    bimg
    bimg_decode
)
set_target_properties(OpenTSDonorHostSmoke PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test-bin)
add_test(NAME donor-host-smoke COMMAND OpenTSDonorHostSmoke --opents-donor-host-smoke)
