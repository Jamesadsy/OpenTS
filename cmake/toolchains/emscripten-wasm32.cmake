# OpenTS browser proof uses the official Emscripten SDK selected by EMSDK.
# The upstream Windows-hosted toolchain labels its 32-bit WebAssembly target "x86" by
# default; bgfx's bx module treats that as a native x86 CPU and adds SSE4.2 flags. Name the
# actual target architecture before including the official toolchain.
if(NOT DEFINED ENV{EMSDK})
    message(FATAL_ERROR "Activate the official emsdk environment before using this toolchain.")
endif()

set(EMSCRIPTEN_SYSTEM_PROCESSOR wasm32 CACHE STRING "Emscripten target architecture" FORCE)
include("$ENV{EMSDK}/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake")
