opents_add_test(TimerPersistence
    NAME timer-persistence
    SOURCES timerpersistence.cpp
    ENGINE
        monotonic.cpp
        mstimer.cpp
        stimer.cpp
        milsectmr.cpp
    DEFINITIONS OPENTS_MONOTONIC_TEST
)
