opents_add_test(SaveTextContract
    NAME save-text-contract
    SOURCES savetextcontract.cpp
    INCLUDES "${CMAKE_SOURCE_DIR}/thirdparty/RmlUi/Include"
    DEFINITIONS
        SAVE_TEXT_FONT_PATH="${CMAKE_SOURCE_DIR}/ui/LatoLatin-Regular.ttf"
        SAVE_TEXT_RML_PATH="${CMAKE_SOURCE_DIR}/ui/missionsave.rml"
        SAVE_TEXT_UI_PATH="${CMAKE_SOURCE_DIR}/ui"
    LIBRARIES rmlui
)
