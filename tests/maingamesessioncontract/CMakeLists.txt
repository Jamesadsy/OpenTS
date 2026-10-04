opents_add_test(MainGameSessionContract
    NAME main-game-session-contract
    SOURCES maingamesessioncontract.cpp ../../code/maingamesession.cpp
)

add_test(NAME main-game-nod-session-contract COMMAND MainGameSessionContract --nod-smoke)

if(NOT EMSCRIPTEN)
    target_sources(MainGameSessionContract PRIVATE cooperativemovie.cpp
        ../../code/initgamesession.cpp ../../code/browsercoord.cpp)
    target_compile_definitions(MainGameSessionContract PRIVATE OPENTS_TEST_BROWSER_MOVIES)
endif()

target_compile_definitions(MainGameSessionContract PRIVATE
    "OPENTS_SOURCE_ROOT=\"${CMAKE_SOURCE_DIR}\""
)
