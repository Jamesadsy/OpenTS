opents_add_test(SaveBrowser
    NAME save-browser
    SOURCES savebrowsercontract.cpp
    ENGINE
        savebrowser.cpp
        savefileenum.cpp
    INCLUDES ${OPENTS_GENERATED_DIR}
    STAMP
)
