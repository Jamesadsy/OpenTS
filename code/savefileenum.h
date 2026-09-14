/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <chrono>
#include <string>
#include <vector>


/*
 * The save browser needs only the name of an eligible file and the timestamp used to order it.
 * Filesystem-specific clocks and native directory records stop at the enumeration boundary.
 */
struct SaveFileRecord
{
	std::string Filename;
	std::chrono::system_clock::time_point ModifiedAt;
};


/*
 * Enumerates the one already-selected save directory. The extension may be written with or
 * without its leading dot and is matched without regard to case. Results are newest first.
 * Missing, inaccessible and individually unreadable entries are omitted.
 */
std::vector<SaveFileRecord> Enumerate_Save_Files(std::string const & directory, char const * extension);
