opents_add_test(CursorStateContract
    NAME cursor-scrollrate-contract
    SOURCES cursorstatecontract.cpp
)

target_compile_definitions(CursorStateContract PRIVATE
    "OPENTS_UI_MODE_ICON_SOURCE=\"${CMAKE_SOURCE_DIR}/code/ui/uimodeicon.cpp\""
    "OPENTS_MSENGINE_SOURCE=\"${CMAKE_SOURCE_DIR}/code/msengine.cpp\""
    "OPENTS_OPTIONS_SOURCE=\"${CMAKE_SOURCE_DIR}/code/options.h\""
    "OPENTS_OPTIONS_CPP_SOURCE=\"${CMAKE_SOURCE_DIR}/code/options.cpp\""
    "OPENTS_CONTROLLER_SOURCE=\"${CMAKE_SOURCE_DIR}/platform/win32compat/src/controller.cpp\""
    "OPENTS_SCROLL_SOURCE=\"${CMAKE_SOURCE_DIR}/code/scroll.cpp\""
    "OPENTS_SIDEBAR_SOURCE=\"${CMAKE_SOURCE_DIR}/code/sidebar.cpp\""
    "OPENTS_DISPLAY_SOURCE=\"${CMAKE_SOURCE_DIR}/code/display.cpp\""
    "OPENTS_MOUSE_CPP_SOURCE=\"${CMAKE_SOURCE_DIR}/code/mouse.cpp\""
    "OPENTS_UNIT_SOURCE=\"${CMAKE_SOURCE_DIR}/code/unit.cpp\""
    "OPENTS_CURSOR_SOURCE=\"${CMAKE_SOURCE_DIR}/code/wincursor.cpp\""
    "OPENTS_CURSOR_HEADER_SOURCE=\"${CMAKE_SOURCE_DIR}/code/wincursor.h\""
    "OPENTS_MOUSE_HEADER_SOURCE=\"${CMAKE_SOURCE_DIR}/code/mouse.h\""
)
