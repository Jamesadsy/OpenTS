set(BGFX_ROOT "${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bgfx")

add_executable(OpenTSWebGLCanvasProof
    "${CMAKE_SOURCE_DIR}/platform/win32compat/src/window.cpp"
    "${CMAKE_SOURCE_DIR}/code/bgfxbackend.cpp"
    webglcanvasproof.cpp
)

target_compile_features(OpenTSWebGLCanvasProof PRIVATE cxx_std_20)
target_compile_definitions(OpenTSWebGLCanvasProof PRIVATE
    NOMINMAX
    BX_CONFIG_DEBUG=$<IF:$<CONFIG:Debug>,1,0>
)
target_include_directories(OpenTSWebGLCanvasProof PRIVATE
    "${CMAKE_SOURCE_DIR}/code"
    "${CMAKE_SOURCE_DIR}/platform/win32compat/src"
    "${CMAKE_SOURCE_DIR}/platform/win32compat/include"
    "${BGFX_ROOT}/include"
    "${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bx/include"
    "${BGFX_ROOT}/examples/common/imgui"
)
target_link_libraries(OpenTSWebGLCanvasProof PRIVATE
    SDL3::SDL3-static
    bgfx
    bx
    bimg
    bimg_decode
)

target_link_options(OpenTSWebGLCanvasProof PRIVATE
    "-sNO_EXIT_RUNTIME=1"
    "-sEXPORTED_FUNCTIONS=_main,_VV124F_Begin,_VV124F_Frame,_VV124F_Resize,_VV124F_Present_Count,_VV124F_Initial_Width,_VV124F_Initial_Height,_VV124F_Resized_Width,_VV124F_Resized_Height,_VV124F_Initial_Window_Width,_VV124F_Initial_Window_Height,_VV124F_Resized_Window_Width,_VV124F_Resized_Window_Height,_VV124F_Backend_Resize_Width,_VV124F_Backend_Resize_Height,_VV124F_Renderer_Name,_VV124F_Finish"
    "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/shell.html"
)

set_target_properties(OpenTSWebGLCanvasProof PROPERTIES
    OUTPUT_NAME "VV124FWebGLCanvasProof"
    SUFFIX ".html"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/webglcanvasproof"
    LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/shell.html"
)
