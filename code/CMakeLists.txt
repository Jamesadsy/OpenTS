# Limit the generated solution to supported configurations.
set(CMAKE_CONFIGURATION_TYPES Debug Release CACHE STRING "" FORCE)

# OpenTS supports 32-bit (x86) builds. A 64-bit build is an unsupported experiment.
if(WIN32 AND NOT CMAKE_SIZEOF_VOID_P EQUAL 4 AND NOT OPENTS_EXPERIMENTAL_X64)
    message(FATAL_ERROR
        "OpenTS must be built as 32-bit x86. Reconfigure with -A Win32. "
        "For the unsupported 64-bit experiment, configure with "
        "-A x64 -DOPENTS_EXPERIMENTAL_X64=ON.")
endif()

#
# ---------------------------------------------------------
# Directories that build as their own project
# ---------------------------------------------------------
#
set(SUBPROJECT_DIRS
    "language"
    "vqalib"
)

#
# ---------------------------------------------------------
# Gather all potential OpenTS files
# ---------------------------------------------------------
#
file(GLOB_RECURSE OPENTS_RAW CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.rc"
)

#
# ---------------------------------------------------------
# Remove files that belong to a subproject
# ---------------------------------------------------------
#
set(OPENTS_SRC "")
foreach(f ${OPENTS_RAW})
    set(EXCLUDED FALSE)
    foreach(dir ${SUBPROJECT_DIRS})
        if(f MATCHES "^${CMAKE_CURRENT_SOURCE_DIR}/${dir}/.*")
            set(EXCLUDED TRUE)
        endif()
    endforeach()

    if(NOT EXCLUDED)
        list(APPEND OPENTS_SRC "${f}")
    endif()
endforeach()

#
# ---------------------------------------------------------
# Create actual OpenTS executable (Win32)
# ---------------------------------------------------------
#
message(STATUS "${PROJECT_NAME}: Creating '${PROJECT_NAME}' executable...")

add_executable(OpenTS WIN32 ${OPENTS_SRC})
target_compile_features(OpenTS PRIVATE cxx_std_20)

#
# ---------------------------------------------------------
# Output paths
# ---------------------------------------------------------
#
message(STATUS "${PROJECT_NAME}: Setting binary output paths...")

set_target_properties(OpenTS PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"

    # Built under the shipped names, so the executable, the symbol file the debug record
    # names, and the map reach the run directory without a rename.
    OUTPUT_NAME_DEBUG "GameD"
    OUTPUT_NAME_RELEASE "Game"
    PDB_NAME_DEBUG "GameD"
    PDB_NAME_RELEASE "Game"
)

#
# ---------------------------------------------------------
# iOS application bundle
# ---------------------------------------------------------
#
# An iOS application is its bundle, and everything the game links is static, so the executable
# target carries the bundle metadata itself instead of handing a binary to a separate shell
# project. Both configurations build OpenTS.app, because signing and installation identify one
# product by name.
if(IOS)
    set(OPENTS_IOS_BUNDLE_ID "org.opents.OpenTS" CACHE STRING "iOS bundle identifier")
    set(OPENTS_IOS_BUNDLE_VERSION "1" CACHE STRING "iOS CFBundleVersion")

    set_target_properties(OpenTS PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/ios/Info.plist.in"
        MACOSX_BUNDLE_BUNDLE_NAME "OpenTS"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${OPENTS_IOS_BUNDLE_ID}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${OPENTS_IOS_BUNDLE_VERSION}"

        OUTPUT_NAME "OpenTS"
        OUTPUT_NAME_DEBUG "OpenTS"
        OUTPUT_NAME_RELEASE "OpenTS"

        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${OPENTS_IOS_BUNDLE_ID}"
        XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
        XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Automatic"
    )

    # The staged copy an installer produces is what gets signed and pushed to a device. It is
    # its own component because the vendored libraries carry install rules of their own, and a
    # staging prefix must hold the bundle alone.
    install(TARGETS OpenTS BUNDLE DESTINATION "." COMPONENT OpenTS)
endif()

set_target_properties(OpenTS PROPERTIES
    VS_DEBUGGER_COMMAND "$<TARGET_FILE:OpenTS>"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:OpenTS>"
    VS_DEBUGGER_COMMAND_ARGUMENTS "-DATADIR=${OPENTS_GAME_DIR}"
)

#
# ---------------------------------------------------------
# Compile flags
# ---------------------------------------------------------
#
message(STATUS "${PROJECT_NAME}: Applying compiler flags...")

if(MSVC OR OPENTS_EXPERIMENTAL_CLANG_CL)
    set(OPENTS_COMPILE_OPTIONS

        # ================= DEBUG =================
        $<$<CONFIG:Debug>:

            # ---------- C++ ----------
            # /arch:SSE2 -- IEEE-754 single precision, no x87 excess precision.
            # /fp:precise -- no reassociation of the engine's accumulations.
            # /utf-8 -- sources and literals are UTF-8 whatever the build machine's code page.
            $<$<COMPILE_LANGUAGE:CXX>:
                /Zi /Od /RTC1 /GR $<$<CXX_COMPILER_ID:MSVC>:/MP> /EHsc /Oy- /MTd /Zc:__cplusplus /arch:SSE2 /fp:precise /utf-8
            >

            # ---------- C ----------
            $<$<COMPILE_LANGUAGE:C>:
                /Zi /Od /RTC1 $<$<C_COMPILER_ID:MSVC>:/MP> /Oy- /arch:SSE2 /fp:precise /utf-8
            >
        >

        # ================= RELEASE =================
        $<$<CONFIG:Release>:

            # ---------- C++ ----------
            # /arch:SSE2 -- IEEE-754 single precision, no x87 excess precision.
            # /fp:precise -- no reassociation of the engine's accumulations.
            # /utf-8 -- sources and literals are UTF-8 whatever the build machine's code page.
            $<$<COMPILE_LANGUAGE:CXX>:
                /Zi /O2 /GF /GR $<$<CXX_COMPILER_ID:MSVC>:/MP> /EHsc /MT /Zc:__cplusplus /arch:SSE2 /fp:precise /utf-8
            >

            # ---------- C ----------
            $<$<COMPILE_LANGUAGE:C>:
                /Zi /O2 /GF $<$<C_COMPILER_ID:MSVC>:/MP> /arch:SSE2 /fp:precise /utf-8
            >
        >
    )
else()
    # The native host build carries no MSVC switch spellings. Only the floating-point
    # contract matters for simulation determinism, so it is the one that is restated.
    set(OPENTS_COMPILE_OPTIONS
        -g
        -ffp-contract=off
        -fno-strict-aliasing
        -Wno-everything
    )
endif()

target_compile_options(OpenTS PRIVATE ${OPENTS_COMPILE_OPTIONS})

#
# ---------------------------------------------------------
# Subprojects
# ---------------------------------------------------------
#
# The VQA player carries its own project, and hands OpenTS its include path along with the
# library, so the movie code is reached the same way from anywhere in the solution.
add_subdirectory(vqalib)

message(STATUS "${PROJECT_NAME}: Adding include paths...")
target_include_directories(OpenTS PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${OPENTS_GENERATED_DIR}"
)

# Windows supplies the window, the message loop and the cursor itself. Every other target
# gets them from the compatibility layer, which carries the headers as well as the code.
if(NOT WIN32)
    target_link_libraries(OpenTS PRIVATE win32compat)

    # Windows answers code page questions from the kernel; everywhere else that service is
    # libiconv, which Apple ships as a separate library rather than inside libc. Emscripten
    # provides iconv through its C library, so it has no separate -liconv to link.
    if(NOT EMSCRIPTEN)
        target_link_libraries(OpenTS PRIVATE iconv)
    endif()
endif()

# The generated build stamp has to exist before anything compiles.
add_dependencies(OpenTS OpenTSBuildStamp)

# Only the renderer's own translation unit sees bgfx, so its headers and the shaders it
# embeds are put on that file rather than on the whole target.
set(BGFX_ROOT "${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bgfx")

# The Debug configuration turns on bx's own debug build, except under Xcode, which refuses a
# definition that varies by configuration on a single source file. That generator takes the
# value bgfx was configured with for every configuration.
if(CMAKE_GENERATOR STREQUAL "Xcode")
    set(OPENTS_BX_CONFIG_DEBUG "BX_CONFIG_DEBUG=$<BOOL:${BX_CONFIG_DEBUG}>")
else()
    set(OPENTS_BX_CONFIG_DEBUG "BX_CONFIG_DEBUG=$<IF:$<CONFIG:Debug>,1,$<BOOL:${BX_CONFIG_DEBUG}>>")
endif()

set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/bgfxbackend.cpp" PROPERTIES
    INCLUDE_DIRECTORIES
        "${BGFX_ROOT}/include;${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bx/include;${BGFX_ROOT}/examples/common/imgui"
    COMPILE_DEFINITIONS "${OPENTS_BX_CONFIG_DEBUG}"
    COMPILE_OPTIONS "$<$<BOOL:${MSVC}>:/Zc:preprocessor>"
)

# The UI shell is scoped the same way: only code/ui sees RmlUi and ImGui, and uirender.cpp
# alone adds bgfx to that, so no other translation unit carries any of their headers or
# build settings. uitexture.cpp reads images through bimg, which bgfx already carries.
file(GLOB OPENTS_UI_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ui/*.cpp")
set_source_files_properties(${OPENTS_UI_SOURCES} PROPERTIES
    INCLUDE_DIRECTORIES
        "${CMAKE_SOURCE_DIR}/thirdparty/RmlUi/Include;${CMAKE_SOURCE_DIR}/thirdparty/imgui"
    COMPILE_OPTIONS "$<$<BOOL:${MSVC}>:/Zc:preprocessor>"
)

# The bitmap font engine derives from RmlUi's own engine, whose header sits in the library's
# Source tree rather than its Include tree, so that one path goes on that one file.
set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/ui/uifont.cpp" APPEND PROPERTY
    INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/thirdparty/RmlUi/Source"
)

set_property(SOURCE
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/uirender.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/ui/uitexture.cpp"
    APPEND PROPERTY INCLUDE_DIRECTORIES
        "${BGFX_ROOT}/include;${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bx/include;${CMAKE_SOURCE_DIR}/thirdparty/bgfx.cmake/bimg/include;${BGFX_ROOT}/examples/common/imgui"
)

set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/ui/uirender.cpp" APPEND PROPERTY
    COMPILE_DEFINITIONS "${OPENTS_BX_CONFIG_DEBUG}"
)
set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/ui/uitexture.cpp" APPEND PROPERTY
    COMPILE_DEFINITIONS "${OPENTS_BX_CONFIG_DEBUG}"
)

# bx rewrites __stdcall while its headers are being parsed by clang-cl.  Force the
# compatibility header into the renderer translation unit as well as bx/bgfx so the
# MSVC standard-library headers that follow still see the Win32 calling convention.
if(OPENTS_EXPERIMENTAL_CLANG_CL AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/bgfxbackend.cpp" APPEND PROPERTY
        COMPILE_OPTIONS "/FI${CMAKE_SOURCE_DIR}/thirdparty/bx-clang-compat.h"
    )
endif()

message(STATUS "${PROJECT_NAME}: Adding compilier definitions...")
target_compile_definitions(OpenTS PRIVATE
    NOMINMAX
)

if(WIN32)
    target_compile_definitions(OpenTS PRIVATE
        WIN32
        _WINDOWS
    )
endif()

#
# ---------------------------------------------------------
# Windows libraries and linker options
# ---------------------------------------------------------
#
message(STATUS "${PROJECT_NAME}: Applying linker options...")

# Required libraries:
#  Comctl32.lib
#  DbgHelp.lib
#  DSound.lib
#  Version.lib
#  Winmm.lib
#  wsock32.lib
#  + all Win32 libs
#

target_link_libraries(OpenTS PRIVATE
    VQALib

    bgfx
    bx
    bimg
    bimg_decode
    miniaudio
    lzo
    rmlui
    imgui
)

if(WIN32)
    target_link_libraries(OpenTS PRIVATE
    comctl32
    dbghelp
    iphlpapi
    psapi
    version
    winmm
    ws2_32
    kernel32 user32 gdi32 winspool comdlg32 advapi32 shell32
    odbc32 odbccp32
    )
endif()

if(MSVC)
    target_link_options(OpenTS PRIVATE
        /SUBSYSTEM:WINDOWS
        /DEBUG
        /MAP
        # Raises the user address space of a 32-bit process from 2GB to 4GB under 64-bit
        # Windows. Pointers may then have the high bit set, so anything holding one in a
        # signed integer must only round-trip the bits and never compare or scale them. A
        # 64-bit image carries the flag already.
        /LARGEADDRESSAWARE
        # Declares the process per-monitor DPI aware, so a scaled desktop does not blur
        # the frame by stretching the whole window.
        "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/sun.manifest"
        /MANIFEST:EMBED
    )
endif()

# The browser process coordinator remains callable after the Emscripten entry point returns.
# The browser owns scheduling; the native process owns startup phases and lifecycle state.
if(EMSCRIPTEN)
	set_property(TARGET OpenTS APPEND PROPERTY LINK_DEPENDS
		"${CMAKE_SOURCE_DIR}/tests/browserstorage/harness.html"
		"${CMAKE_SOURCE_DIR}/tests/browserstorage/harness.js"
		"${CMAKE_SOURCE_DIR}/tests/browserstorage/runtime-input.js"
		"${CMAKE_SOURCE_DIR}/tests/browserstorage/browserstorage.js"
	)
	set_target_properties(OpenTS PROPERTIES
		OUTPUT_NAME OpenTS
		OUTPUT_NAME_DEBUG OpenTS
		OUTPUT_NAME_RELEASE OpenTS
	)
    target_link_options(OpenTS PRIVATE
        "-sNO_EXIT_RUNTIME=1"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sFORCE_FILESYSTEM=1"
        "-sEXPORTED_RUNTIME_METHODS=UTF8ToString,FS,callMain"
		"-sEXPORTED_FUNCTIONS=_main,_OpenTS_Browser_Begin,_OpenTS_Browser_Step,_OpenTS_Browser_Status,_OpenTS_Browser_Finish,_OpenTS_Browser_Begin_Count,_OpenTS_Browser_Begin_Result,_OpenTS_Browser_Main_Returned,_OpenTS_Browser_Main_Entry_Returned,_OpenTS_Browser_Step_Count,_OpenTS_Browser_WaitFocus_Yields,_OpenTS_Browser_MainGame_Began,_OpenTS_Browser_MainGame_Steps,_OpenTS_Browser_Shutdown_Drain_Passes,_OpenTS_Browser_OwnerData_Boundary,_OpenTS_Browser_Bootstrap_Mixfiles_Opened,_OpenTS_Browser_Secondary_Mixfiles_Opened,_OpenTS_Browser_Frontend_Ready,_OpenTS_Browser_Request_Proof_Stop,_OpenTS_Browser_Fault_Code,_OpenTS_Browser_Finish_Count,_OpenTS_124H_Browser_Fault_Finish_Proof,_OpenTS_Browser_Renderer_Initialized,_OpenTS_Browser_Renderer_Name,_OpenTS_Browser_Bootstrap_Stage,_OpenTS_Browser_Finish_Stage,_OpenTS_Browser_ProgEnd_Stage,_OpenTS_Browser_Telemetry_Increment,_OpenTS_Browser_Telemetry_Counter,_OpenTS_Browser_Telemetry_Set,_OpenTS_Browser_Telemetry_Value,_OpenTS_Browser_Telemetry_Next_Event,_OpenTS_Browser_Step_Active,_OpenTS_Browser_Step_Entry_Ms,_OpenTS_Browser_Step_Exit_Ms,_OpenTS_Browser_Last_Step_Result,_OpenTS_Browser_Theme_Requested,_OpenTS_Browser_Theme_Name,_OpenTS_Browser_Audio_Available,_OpenTS_Browser_Audio_Device_Available,_OpenTS_Browser_Audio_Device_Open,_OpenTS_Browser_Audio_Device_Running,_OpenTS_Browser_Audio_Feeder_Passes,_OpenTS_Browser_Audio_Feeder_Running,_OpenTS_124H_Storage_Mark_Ready,_OpenTS_124H_Storage_Is_Ready,_OpenTS_124H_Data_Probe,_OpenTS_124H_AtOwnerDataBoundary,_OpenTS_124H_Data_Root,_OpenTS_124H_Data_Root_Stable,_OpenTS_124H_Data_Probe_Passed,_OpenTS_124H_Boundary_Probe_Passed,_OpenTS_124H_Boundary_Probe_Count,_OpenTS_124H_Root_Entry_Count,_OpenTS_124H_Nested_Entry_Count,_OpenTS_124H_Boundary_Sentinel_Size,_OpenTS_124H_User_Write,_OpenTS_124H_User_Readback_Hex,_OpenTS_124H_User_Readback_Size,_OpenTS_124H_User_Remove,_OpenTS_126_Storage_Mark_Ready,_OpenTS_126_Storage_Is_Ready,_OpenTS_126_Data_Probe,_OpenTS_126_Data_Probe_Passed"
        "-lidbfs.js"
    )
    add_custom_command(TARGET OpenTS POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/tests/browserstartup/harness.html"
            "$<TARGET_FILE_DIR:OpenTS>/OpenTS-browser-proof.html"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/tests/browserstartup/harness.js"
            "$<TARGET_FILE_DIR:OpenTS>/OpenTS-browser-proof.js"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/tests/browserstorage/harness.html"
            "$<TARGET_FILE_DIR:OpenTS>/OpenTS-browser-storage-proof.html"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/tests/browserstorage/harness.js"
            "$<TARGET_FILE_DIR:OpenTS>/OpenTS-browser-storage-proof.js"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/tests/browserstorage/runtime-input.js"
            "$<TARGET_FILE_DIR:OpenTS>/OpenTS-browser-storage-input.js"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/tests/browserstorage/browserstorage.js"
            "$<TARGET_FILE_DIR:OpenTS>/OpenTS-browser-storage-runtime.js"
        COMMENT "Staging the bounded browser process and storage proof harnesses"
        VERBATIM
    )
endif()

#
# ---------------------------------------------------------
# Visual Studio source groups
# ---------------------------------------------------------
#
message(STATUS "${PROJECT_NAME}: Setting up Visual Studio filters...")

# Init Source Files (_*.cpp)
foreach(f ${OPENTS_SRC})
    if(f MATCHES "[/\\\\]_.*\\.cpp$")
        source_group("Source Files\\Init Source Files" FILES ${f})
    endif()
endforeach()

# Init Header Files (_*.h)
foreach(f ${OPENTS_SRC})
    if(f MATCHES "[/\\\\]_.*\\.h$")
        source_group("Header Files\\Init Header Files" FILES ${f})
    endif()
endforeach()

# Define Header Files (*.hh)
foreach(f ${OPENTS_SRC})
    if(f MATCHES "\\.hh$")
        source_group("Header Files\\Define Header Files" FILES ${f})
    endif()
endforeach()

# Resource Files
foreach(f ${OPENTS_SRC})
    if(f MATCHES "\\.(ico|cur|bmp|gif|jpe?g|rc)$")
        source_group("Resource Files" FILES ${f})
    endif()
endforeach()

# The interface headers the locomotors are written against.
source_group("Interface Files" FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/iflyctrl.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/iloco.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ipiggy.h"
)

# General source files
source_group("Source Files" REGULAR_EXPRESSION ".*\\.(c|cpp)$")
source_group("Header Files" REGULAR_EXPRESSION ".*\\.(h|hpp)$")

#
# ---------------------------------------------------------
# Populate the game data directory
# ---------------------------------------------------------
#
# The build no longer copies itself into the game directory, but the shipped UI documents
# are game data the engine resolves under the data directory, so they are placed there.
message(STATUS "${PROJECT_NAME}: Creating post-build commands...")

# The shipped UI documents, styles and font travel with the game data. They are read
# through the game's file system, so this directory is what the search path points at. An
# iOS bundle is the read-only half of that search path and carries them itself.
if(IOS)
    # The string table code/language generates goes in with them, since the bundle is the
    # directory the game finds beside its executable.
    add_custom_command(TARGET OpenTS POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/ui"
            "$<TARGET_BUNDLE_DIR:OpenTS>/ui"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENTS_GENERATED_DIR}/Language.dat"
            "$<TARGET_BUNDLE_DIR:OpenTS>/Language.dat"
    )
else()
    add_custom_command(TARGET OpenTS POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/ui"
            "${OPENTS_GAME_DIR}/ui"
    )
endif()

#
# ---------------------------------------------------------
# Visual Studio startup project
# ---------------------------------------------------------
#
set_property(GLOBAL PROPERTY VS_STARTUP_PROJECT OpenTS)

# ---------------------------------------------------------
# Add the .natvis file for Visual Studio debugging
# ---------------------------------------------------------
set(NATVIS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/sun.natvis")
source_group("Natvis Files" FILES ${NATVIS_FILE})
target_sources(OpenTS PRIVATE ${NATVIS_FILE})
if(MSVC)
    target_link_options(OpenTS PRIVATE "/NATVIS:${NATVIS_FILE}")
endif()
