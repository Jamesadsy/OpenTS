# The directory handling is compiled straight into the harness, along with the file classes
# and the INI reader it stands on. It lives outside code/ so that the recursive glob building
# OpenTS cannot pick this target's entry point up.

set(GAMEDIRS_INCLUDES ${OPENTS_GENERATED_DIR})
set(GAMEDIRS_LIBRARIES)
set(GAMEDIRS_DEFINITIONS)
if(WIN32)
    set(GAMEDIRS_DEFINITIONS WIN32 _WINDOWS _MBCS NOMINMAX)
endif()
if(NOT WIN32)
    list(APPEND GAMEDIRS_INCLUDES ${CMAKE_SOURCE_DIR}/platform/win32compat/include)
    list(APPEND GAMEDIRS_LIBRARIES win32compat)
endif()

opents_add_test(GameDirs
    NAME gamedirs
    SOURCES gamedirscontract.cpp
    ENGINE
        gamedirs.cpp
        cdfile.cpp
        bfiofile.cpp
        rawfile.cpp
        file.cpp
        file_posix.cpp
        file_win.cpp
        dbgprint.cpp
        ini.cpp
        utf8.cpp
        b64pipe.cpp
        b64straw.cpp
        base64.cpp
        buff.cpp
        crc.cpp
        cstraw.cpp
        int.cpp
        mpmath.cpp
        pipe.cpp
        pk.cpp
        straw.cpp
        trim.cpp
        xpipe.cpp
        xstraw.cpp
    INCLUDES ${GAMEDIRS_INCLUDES}
    DEFINITIONS ${GAMEDIRS_DEFINITIONS}
    UTF8
    STAMP
    LIBRARIES ${GAMEDIRS_LIBRARIES}
)
