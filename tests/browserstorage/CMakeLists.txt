opents_add_test(BrowserStorageContract
    NAME browser-storage-contract
    SOURCES browserstoragecontract.cpp
)

target_compile_definitions(BrowserStorageContract PRIVATE
    "OPENTS_SOURCE_ROOT=\"${CMAKE_SOURCE_DIR}\""
)

find_program(OPENTS_BROWSER_INPUT_NODE node)
if(OPENTS_BROWSER_INPUT_NODE)
    add_test(NAME browser-runtime-input-js
        COMMAND "${OPENTS_BROWSER_INPUT_NODE}" --test
            "${CMAKE_CURRENT_SOURCE_DIR}/runtime-input.test.mjs")
endif()
