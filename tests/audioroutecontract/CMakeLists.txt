file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" OPENTS_SOURCE_DIR_CMAKE)

opents_add_test(IOSAudioRouteContract
    NAME ios-audio-route-contract
    SOURCES audioroutecontract.cpp
    DEFINITIONS OPENTS_SOURCE_DIR="${OPENTS_SOURCE_DIR_CMAKE}"
)
