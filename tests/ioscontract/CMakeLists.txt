add_executable(IOSPathContract
    iospathcontract.cpp
    ${CMAKE_SOURCE_DIR}/platform/win32compat/src/iospaths.cpp
)

target_compile_features(IOSPathContract PRIVATE cxx_std_20)
target_include_directories(IOSPathContract PRIVATE
    ${CMAKE_SOURCE_DIR}/platform/win32compat/src
)
add_test(NAME ios-path-contract COMMAND IOSPathContract)

add_executable(IOSTouchContract
    iostouchcontract.cpp
)

target_compile_features(IOSTouchContract PRIVATE cxx_std_20)
target_compile_definitions(IOSTouchContract PRIVATE OPENTS_TOUCH_TEST)
target_include_directories(IOSTouchContract PRIVATE
    ${CMAKE_SOURCE_DIR}/platform/win32compat/src
    ${CMAKE_SOURCE_DIR}/platform/win32compat/include
)
target_link_libraries(IOSTouchContract PRIVATE win32compat)
add_test(NAME ios-touch-contract COMMAND IOSTouchContract)
