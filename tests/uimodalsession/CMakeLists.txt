opents_add_test(UIModalSessionContract
    NAME ui-modal-session-contract
    SOURCES uimodalsessioncontract.cpp ../../code/ui/uimodalsession.cpp ../../code/mainloopsession.cpp
)

target_compile_definitions(UIModalSessionContract PRIVATE
    "OPENTS_SOURCE_ROOT=\"${CMAKE_SOURCE_DIR}\""
)
