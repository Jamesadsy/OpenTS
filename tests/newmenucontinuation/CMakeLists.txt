opents_add_test(NewMenuContinuationContract
    NAME newmenu-continuation-contract
    SOURCES newmenucontinuationcontract.cpp
)

target_compile_definitions(NewMenuContinuationContract PRIVATE
    "OPENTS_SOURCE_ROOT=\"${CMAKE_SOURCE_DIR}\""
)

opents_add_test(AnimationWaitContract
    NAME animation-wait-contract
    SOURCES animationwaitcontract.cpp
)
set_tests_properties(animation-wait-contract PROPERTIES
    PASS_REGULAR_EXPRESSION "Persistent backdrop completion checks passed\\.")
