opents_add_test(BrowserStorageContract
    NAME browser-storage-contract
    SOURCES browserstoragecontract.cpp
)

target_compile_definitions(BrowserStorageContract PRIVATE
    "OPENTS_SOURCE_ROOT=\"${CMAKE_SOURCE_DIR}\""
)
