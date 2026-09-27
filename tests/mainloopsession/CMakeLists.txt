opents_add_test(MainLoopSessionContract
    NAME main-loop-session-contract
    SOURCES mainloopsessioncontract.cpp ../../code/mainloopsession.cpp
)

target_compile_definitions(MainLoopSessionContract PRIVATE
    "OPENTS_SOURCE_ROOT=\"${CMAKE_SOURCE_DIR}\""
)
