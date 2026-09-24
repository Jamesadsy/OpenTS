opents_add_test(SidebarFeedbackContract
    NAME sidebar-feedback-contract
    SOURCES sidebarfeedbackcontract.cpp
)

target_compile_definitions(SidebarFeedbackContract PRIVATE
    "OPENTS_SIDEBAR_SOURCE=\"${CMAKE_SOURCE_DIR}/code/sidebar.cpp\""
)
