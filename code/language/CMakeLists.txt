file(GLOB_RECURSE LANG_SRC CONFIGURE_DEPENDS
    "*.cpp" "*.c" "*.cc"
    "*.h" "*.hpp" "*.hm"
)

if(WIN32)
    file(GLOB_RECURSE LANG_RC CONFIGURE_DEPENDS "*.rc")
    list(APPEND LANG_SRC ${LANG_RC})
endif()

add_library(Language SHARED ${LANG_SRC})
target_compile_features(Language PRIVATE cxx_std_20)
if(WIN32)
    target_compile_definitions(Language PRIVATE NOMINMAX)
endif()

# The version resource is stamped from the generated headers, which the resource compiler
# reaches through the target's include paths.
target_include_directories(Language PRIVATE "${OPENTS_GENERATED_DIR}")
add_dependencies(Language OpenTSBuildStamp)

# The game loads its strings from this library at runtime, so building the executable alone
# would leave a stale Language.dll beside it.
add_dependencies(OpenTS Language)

if(MSVC)
    target_link_options(Language PRIVATE "/NOENTRY" "/NODEFAULTLIB")
endif()

# The game loads this library by name from its own directory, so it is built beside the
# executable rather than copied there.
set_target_properties(Language PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
