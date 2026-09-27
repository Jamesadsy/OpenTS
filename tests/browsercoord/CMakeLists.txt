opents_add_test(BrowserCoordinatorContract
    NAME browser-coordinator-contract
    SOURCES browsercoordcontract.cpp ../../code/browsercoord.cpp
)

target_compile_definitions(BrowserCoordinatorContract PRIVATE
    "OPENTS_SOURCE_ROOT=\"${CMAKE_SOURCE_DIR}\""
)
