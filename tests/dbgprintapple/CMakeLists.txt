# Apple G2 owns a portable diagnostic contract test.  The real Apple implementation is
# compiled into this harness so the test does not substitute or copy its behaviour.

opents_add_test(DebugPrintApple
    NAME dbgprintapple
    SOURCES dbgprintapple.cpp
    ENGINE
        dbgprint_apple.cpp
)
