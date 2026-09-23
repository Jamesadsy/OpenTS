opents_add_test(CursorStateContract
    NAME cursor-scrollrate-contract
    SOURCES cursorstatecontract.cpp
)

target_compile_definitions(CursorStateContract PRIVATE
    "OPENTS_UI_MODE_ICON_SOURCE=\"${CMAKE_SOURCE_DIR}/code/ui/uimodeicon.cpp\""
)
