opents_add_test(ControllerFocusContract
    NAME controller-focus-contract
    SOURCES controllerfocuscontract.cpp
    INCLUDES "${CMAKE_SOURCE_DIR}/platform/win32compat/src"
)

target_compile_definitions(ControllerFocusContract PRIVATE
    "OPENTS_SOURCE_ROOT=\"${CMAKE_SOURCE_DIR}\""
    "OPENTS_UI_SHELL_SOURCE=\"${CMAKE_SOURCE_DIR}/code/ui/uishell.cpp\""
)
