/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "languageprovider.hh"

#include "language_provider_generated.hh"
#include "opents_version.h"

#include <cstdio>
#include <cstring>


bool Language_Provider_Load_String(int id, char * destination, std::size_t capacity)
{
	if (destination == NULL || capacity == 0) {
		return(false);
	}

	char const * text = Language_Native_String(id);
	if (text == NULL) {
		return(false);
	}

	std::strncpy(destination, text, capacity);
	destination[capacity - 1] = '\0';
	return(true);
}


bool Init_Language_Resources(bool)
{
	return(true);
}


void Get_Language_Version(char * version_string)
{
	if (version_string != NULL) {
		std::sprintf(version_string, "Language: built-in English resources %s (%s)",
			OPENTS_VERSION_DISPLAY, Language_Native_Source_SHA256());
	}
}
