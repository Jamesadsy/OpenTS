opents_add_test(KeyboardContract
    NAME keyboard-contract
    SOURCES keyboardcontract.cpp
    ENGINE keyboard.cpp
    DEFINITIONS OPENTS_IOS
)

if(NOT WIN32)
    target_link_libraries(KeyboardContract PRIVATE win32compat)
endif()
