/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "ui/stringtranslation.h"
#include "portabletable.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace {

struct NamedString { char const * name; int id; };
NamedString const GeneratedNames[] = {
#define OPENTS_STRING_NAME(name, id) { name, id },
#include "stringnames.hh"
#undef OPENTS_STRING_NAME
};

std::unordered_map<std::string, int> NameIds;
std::unordered_map<int, std::string> PortableStrings;

bool Lookup(std::string const & name, std::string & text)
{
	auto const found_name = NameIds.find(name);
	if (found_name == NameIds.end()) return(false);
	auto const found_text = PortableStrings.find(found_name->second);
	if (found_text == PortableStrings.end()) return(false);
	text = found_text->second;
	return(true);
}

bool Read_Language_Table(void)
{
	std::ifstream input(OPENTS_PORTABLE_LANGUAGE_TABLE, std::ios::binary);
	std::string content((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
	OpenTSPortableStrings::StringMap strings;
	if (!OpenTSPortableStrings::Parse_Portable_String_Table(content, strings)) return(false);
	for (auto const & [id, text] : strings) PortableStrings.emplace(static_cast<int>(id), text);
	return(true);
}

void Collect_References(std::filesystem::path const & root, std::unordered_set<std::string> & names)
{
	static std::regex const token(R"(\[\[(TXT_[A-Z0-9_]+)\]\])");
	for (auto const & entry : std::filesystem::recursive_directory_iterator(root)) {
		if (!entry.is_regular_file() || entry.path().extension() != ".rml") continue;
		std::ifstream file(entry.path(), std::ios::binary);
		std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		for (std::sregex_iterator at(text.begin(), text.end(), token), end; at != end; ++at) {
			names.insert((*at)[1].str());
		}
	}
}

}

int main(void)
{
	for (NamedString const & name : GeneratedNames) NameIds.emplace(name.name, name.id);
	if (!Read_Language_Table()) {
		std::cerr << "portable Language.dat could not be parsed\n";
		return(2);
	}

	std::unordered_set<std::string> references;
	Collect_References(OPENTS_SOURCE_UI_DIR, references);
	int failures = 0;
	for (std::string const & name : references) {
		auto const id = NameIds.find(name);
		bool const known = id != NameIds.end();
		std::string fetched;
		bool const nonempty = known && Lookup(name, fetched) && !fetched.empty();
		std::string rendered;
		int const replacements = Translate_Rml_String(rendered, "[[" + name + "]]", Lookup);
		bool const browser_translation = replacements == 1 && !rendered.empty();
		std::cout << name << ": generated=" << (known ? "yes" : "no")
			<< ", portable=" << (nonempty ? "non-empty" : "missing/empty")
			<< ", translation=" << (browser_translation ? "non-empty" : "FAILED") << '\n';
		if (!known || !nonempty || !browser_translation) failures++;
	}
	std::cout << "RmlUi token audit: " << references.size() << " unique packaged tokens, "
		<< failures << " failures\n";
	return(failures == 0 && !references.empty() ? 0 : 1);
}
