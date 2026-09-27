opents_add_test(UIFrontendContinuationContract
    NAME ui-frontend-continuation-contract
    SOURCES uifrontendcontinuationcontract.cpp
)

target_compile_definitions(UIFrontendContinuationContract PRIVATE
    "OPENTS_SOURCE_ROOT=\"${CMAKE_SOURCE_DIR}\""
)
