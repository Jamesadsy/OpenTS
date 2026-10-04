set(RMLUI_PORTABLE_LANGUAGE_TABLE "${CMAKE_CURRENT_BINARY_DIR}/Language.dat")

add_custom_command(
	OUTPUT "${RMLUI_PORTABLE_LANGUAGE_TABLE}"
	COMMAND ${CMAKE_COMMAND}
		"-DRC_FILE=${CMAKE_SOURCE_DIR}/code/language/language.rc"
		"-DHEADER_FILE=${CMAKE_SOURCE_DIR}/code/language/language.h"
		"-DOUTPUT=${RMLUI_PORTABLE_LANGUAGE_TABLE}"
		-P "${CMAKE_SOURCE_DIR}/cmake/StringTable.cmake"
	DEPENDS
		"${CMAKE_SOURCE_DIR}/code/language/language.rc"
		"${CMAKE_SOURCE_DIR}/code/language/language.h"
		"${CMAKE_SOURCE_DIR}/cmake/StringTable.cmake"
	VERBATIM
)
add_custom_target(RmlUiPortableLanguageTable DEPENDS "${RMLUI_PORTABLE_LANGUAGE_TABLE}")

opents_add_test(RmlUiTranslationContract
	NAME rmlui-translation-token-contract
	SOURCES rmluitranslationcontract.cpp
	ENGINE ui/stringtranslation.cpp
	INCLUDES "${OPENTS_GENERATED_DIR}" "${CMAKE_SOURCE_DIR}/platform/win32compat/src"
)
target_compile_definitions(RmlUiTranslationContract PRIVATE
	"OPENTS_SOURCE_UI_DIR=\"${CMAKE_SOURCE_DIR}/ui\""
	"OPENTS_PORTABLE_LANGUAGE_TABLE=\"${RMLUI_PORTABLE_LANGUAGE_TABLE}\""
)
add_dependencies(RmlUiTranslationContract OpenTSStringNames RmlUiPortableLanguageTable)
