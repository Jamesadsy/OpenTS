opents_add_test(AppleProfileContract
    NAME apple-profile-contract
    SOURCES profilecontract.cpp
    INCLUDES ${OPENTS_GENERATED_DIR}
    DEFINITIONS WIN32 _WINDOWS _MBCS NOMINMAX
)
