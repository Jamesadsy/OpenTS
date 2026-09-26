/*******************************************************************************
 *                                O P E N T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 *******************************************************************************/

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

namespace
{

int Failures = 0;


void Check(bool condition, char const * what)
{
	std::printf("%-76s %s\n", what, condition ? "ok" : "FAILED");
	if (!condition) {
		Failures++;
	}
}


std::string Read_Source(char const * path)
{
	std::ifstream source(std::string(OPENTS_SOURCE_DIR) + path);
	return(std::string(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>()));
}


std::string Value_For_Key(std::string const & plist, char const * key)
{
	std::string const key_tag = std::string("<key>") + key + "</key>";
	size_t const key_position = plist.find(key_tag);
	if (key_position == std::string::npos) {
		return(std::string());
	}

	size_t const value_start = plist.find("<string>", key_position + key_tag.size());
	if (value_start == std::string::npos) {
		return(std::string());
	}
	size_t const content_start = value_start + std::string("<string>").size();
	size_t const value_end = plist.find("</string>", content_start);
	if (value_end == std::string::npos) {
		return(std::string());
	}
	return(plist.substr(content_start, value_end - content_start));
}


void Test_Display_Name_And_Stable_Identity(void)
{
	std::string const plist = Read_Source("/cmake/ios/Info.plist.in");
	std::string const cmake = Read_Source("/code/CMakeLists.txt");
	std::string const package = Read_Source("/scripts/build/ios/package-ios.sh");
	Check(!plist.empty() && !cmake.empty() && !package.empty(), "iOS bundle and packaging sources are readable");

	Check(Value_For_Key(plist, "CFBundleDisplayName") == "CnC TS",
		"the iOS CFBundleDisplayName resolves to CnC TS");
	Check(Value_For_Key(plist, "CFBundleName") == "${MACOSX_BUNDLE_BUNDLE_NAME}"
		&& cmake.find("MACOSX_BUNDLE_BUNDLE_NAME \"OpenTS\"") != std::string::npos,
		"the internal bundle name remains OpenTS");
	Check(Value_For_Key(plist, "CFBundleIdentifier") == "${MACOSX_BUNDLE_GUI_IDENTIFIER}"
		&& cmake.find("set(OPENTS_IOS_BUNDLE_ID \"org.opents.OpenTS\" CACHE STRING \"iOS bundle identifier\")")
			!= std::string::npos
		&& cmake.find("MACOSX_BUNDLE_GUI_IDENTIFIER \"${OPENTS_IOS_BUNDLE_ID}\"") != std::string::npos
		&& cmake.find("XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER \"${OPENTS_IOS_BUNDLE_ID}\"")
			!= std::string::npos,
		"the bundle identifier source remains org.opents.OpenTS");
	Check(Value_For_Key(plist, "CFBundleExecutable") == "${MACOSX_BUNDLE_EXECUTABLE_NAME}"
		&& cmake.find("add_executable(OpenTS WIN32 ${OPENTS_SRC})") != std::string::npos
		&& cmake.find("OUTPUT_NAME \"OpenTS\"\n        OUTPUT_NAME_DEBUG \"OpenTS\"\n        OUTPUT_NAME_RELEASE \"OpenTS\"")
			!= std::string::npos,
		"the target, executable, and iOS output names remain OpenTS");
	Check(package.find("--destination \"Documents/OpenTS\"") != std::string::npos,
		"the app data destination remains Documents/OpenTS");
}

}


int main(void)
{
	Test_Display_Name_And_Stable_Identity();
	std::printf("%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
