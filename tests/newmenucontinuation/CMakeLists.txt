opents_add_test(NewMenuContinuationContract
    NAME newmenu-continuation-contract
    SOURCES newmenucontinuationcontract.cpp
)

target_compile_definitions(NewMenuContinuationContract PRIVATE
    "OPENTS_SOURCE_ROOT=\"${CMAKE_SOURCE_DIR}\""
)
