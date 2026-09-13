/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "language/language.h"
#include "language_provider_generated.hh"

#if defined(__APPLE__)
#include "languageprovider.hh"
#endif

#include <cstdio>
#include <cstring>

namespace {

int Failures = 0;


void Check_String(int id, char const * expected, char const * name)
{
	char const * text = Language_Native_String(id);
	bool const canonical = text != nullptr && std::strcmp(text, expected) == 0;

#if defined(__APPLE__)
	char buffer[2048] = {};
	bool const provider = Language_Provider_Load_String(id, buffer, sizeof(buffer))
		&& std::strcmp(buffer, expected) == 0;
	bool const passed = canonical && provider;
#else
	bool const passed = canonical;
#endif

	std::printf("%-76s %s\n", name, passed ? "ok" : "FAILED");
	if (!passed) {
		Failures++;
	}
}

}


int main(void)
{
	Check_String(TXT_COPYRIGHT, "\xC2\xA9 2000 Electronic Arts, All Rights Reserved", "ordinary string ID preserves UTF-8 source bytes");
	Check_String(TXT_SHORT_TITLE, "Tiberian Sun", "ordinary title ID preserves canonical text");
	Check_String(TXT_CONNECTION_QUALITY_RUNG, "%s (rung %u)", "high string ID preserves canonical text and format tokens");
	std::printf("\n%s\n", Failures == 0 ? "PASSED" : "FAILED");
	return(Failures == 0 ? 0 : 1);
}
