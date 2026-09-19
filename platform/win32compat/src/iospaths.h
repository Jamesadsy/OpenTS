/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <filesystem>
#include <string>
#include <vector>

// The iOS container has one read/search root and one writable player root. Keeping the
// construction here makes the host injection, host logs and touch diagnostics use one
// deterministic policy without teaching the helper anything about UIKit or owner data.
struct IOSPaths
{
	std::filesystem::path Root;
	std::filesystem::path Data;
	std::filesystem::path User;

	bool Valid(void) const { return(!Root.empty() && !Data.empty() && !User.empty()); }
};

IOSPaths Build_IOS_Paths(char const * home);
bool Create_IOS_Paths(IOSPaths const & paths);
std::vector<std::string> IOS_Command_Line_Arguments(IOSPaths const & paths);
std::filesystem::path IOS_Log_Directory(IOSPaths const & paths);
std::filesystem::path IOS_Touch_Log_Directory(IOSPaths const & paths);
