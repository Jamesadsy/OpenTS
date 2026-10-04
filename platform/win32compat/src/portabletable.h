/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#pragma once

#include <cstdlib>
#include <string>
#include <unordered_map>
#include <utility>

namespace OpenTSPortableStrings {

using StringMap = std::unordered_map<unsigned int, std::string>;

inline bool Parse_Portable_String_Table(std::string const & content, StringMap & strings)
{
	strings.clear();
	std::size_t cursor = 0;
	auto next_line = [&content, &cursor](std::string & line) {
		if (cursor >= content.size()) return(false);
		std::size_t const end = content.find('\n', cursor);
		if (end == std::string::npos) return(false);
		line.assign(content, cursor, end - cursor);
		cursor = end + 1;
		if (!line.empty() && line.back() == '\r') line.pop_back();
		return(true);
	};

	std::string line;
	if (!next_line(line) || line != "OPENTS-STRINGS 1") return(false);
	if (!next_line(line)) return(false);
	char * count_end = nullptr;
	long const count = std::strtol(line.c_str(), &count_end, 10);
	if (count_end == line.c_str() || *count_end != '\0' || count < 0 || count > 65536) return(false);

	for (long record = 0; record < count; ++record) {
		if (!next_line(line)) return(false);
		char * id_end = nullptr;
		unsigned long const id = std::strtoul(line.c_str(), &id_end, 10);
		if (id_end == line.c_str() || *id_end != ' ') return(false);
		char * length_end = nullptr;
		unsigned long const length = std::strtoul(id_end + 1, &length_end, 10);
		if (length_end == id_end + 1 || *length_end != '\0'
			|| length > (content.size() - cursor) / 2) return(false);

		std::string decoded;
		decoded.reserve(static_cast<std::size_t>(length));
		for (unsigned long byte = 0; byte < length; ++byte) {
			char const high = content[cursor++];
			char const low = content[cursor++];
			auto const nibble = [](char value) -> int {
				if (value >= '0' && value <= '9') return(value - '0');
				if (value >= 'A' && value <= 'F') return(value - 'A' + 10);
				if (value >= 'a' && value <= 'f') return(value - 'a' + 10);
				return(-1);
			};
			int const upper = nibble(high);
			int const lower = nibble(low);
			if (upper < 0 || lower < 0) return(false);
			decoded.push_back(static_cast<char>((upper << 4) | lower));
		}
		strings[static_cast<unsigned int>(id)] = std::move(decoded);
		if (cursor < content.size() && content[cursor] == '\r') ++cursor;
		if (cursor >= content.size() || content[cursor] != '\n') return(false);
		++cursor;
	}
	return(true);
}

} // namespace OpenTSPortableStrings
