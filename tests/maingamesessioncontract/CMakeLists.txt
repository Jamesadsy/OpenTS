opents_add_test(MainGameSessionContract
    NAME main-game-session-contract
    SOURCES maingamesessioncontract.cpp ../../code/maingamesession.cpp
)

target_compile_definitions(MainGameSessionContract PRIVATE
    "OPENTS_SOURCE_ROOT=\"${CMAKE_SOURCE_DIR}\""
)
