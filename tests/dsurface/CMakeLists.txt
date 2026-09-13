# DSurface is a CPU RGB565 contract.  This harness deliberately provides no native window,
# device context or game data, so the pixel semantics stay verifiable on every supported host.
opents_add_test(DSurface
    NAME dsurface
    SOURCES dsurfacetest.cpp
    ENGINE
        dsurface.cpp
        dsurface_windows_gdi.cpp
        xsurface.cpp
        blit.cpp
        buff.cpp
        zbuffer.cpp
        abuffer.cpp
        rgb.cpp
        _zbuffer.cpp
    INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}
    DEFINITIONS WIN32 _WINDOWS _MBCS NOMINMAX
)
