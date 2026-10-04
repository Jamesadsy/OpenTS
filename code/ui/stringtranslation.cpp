/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "ui/stringtranslation.h"

int Translate_Rml_String(std::string & translated, std::string const & input,
	RmlStringLookup lookup)
{
	translated.clear();
	int replaced = 0;
	std::size_t position = 0;

	while (position < input.size()) {
		std::size_t open = input.find("[[", position);
		if (open == std::string::npos) break;

		std::size_t close = input.find("]]", open + 2);
		if (close == std::string::npos) break;

		std::string text;
		std::string const name = input.substr(open + 2, close - open - 2);
		translated.append(input, position, open - position);

		if (lookup != nullptr && lookup(name, text)) {
			translated.append(text);
			replaced++;
		} else {
			translated.append(input, open, close + 2 - open);
		}
		position = close + 2;
	}

	translated.append(input, position, std::string::npos);
	return(replaced);
}
