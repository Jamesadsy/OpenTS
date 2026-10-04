file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" OPENTS_SOURCE_DIR_CMAKE)

opents_add_test(MovieAudioQuiesceContract
    NAME movie-audio-quiesce-contract
    SOURCES movieaudioquiescecontract.cpp
    DEFINITIONS OPENTS_SOURCE_DIR="${OPENTS_SOURCE_DIR_CMAKE}"
)
