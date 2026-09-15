opents_add_test(SaveBrowser
    NAME save-browser
    SOURCES savebrowsercontract.cpp
    ENGINE
        savebrowser.cpp
        savefileenum.cpp
    INCLUDES ${OPENTS_GENERATED_DIR}
    DEFINITIONS WIN32 _WINDOWS _MBCS NOMINMAX
    STAMP
)
