add_executable(LanguageProviderDataTest languageprovider.cpp)
target_compile_features(LanguageProviderDataTest PRIVATE cxx_std_20)
target_include_directories(LanguageProviderDataTest PRIVATE
    "${CMAKE_SOURCE_DIR}/code"
    "${OPENTS_GENERATED_DIR}"
)
add_dependencies(LanguageProviderDataTest OpenTSLanguageProviderData)

if(APPLE)
    target_sources(LanguageProviderDataTest PRIVATE "${CMAKE_SOURCE_DIR}/code/languageprovider_apple.cpp")
    add_dependencies(LanguageProviderDataTest OpenTSBuildStamp)
endif()

add_test(NAME languageprovider COMMAND LanguageProviderDataTest)
