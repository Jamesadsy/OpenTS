/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#pragma once

#include <string>

using RmlStringLookup = bool (*)(std::string const & name, std::string & text);

// Shared production token expansion, independent of RmlUi so the packaged resource path
// can be verified by a deterministic native contract.
int Translate_Rml_String(std::string & translated, std::string const & input,
	RmlStringLookup lookup);
