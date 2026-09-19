/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "iospaths.h"

namespace
{

bool Ensure_Directory(std::filesystem::path const & path)
{
	std::error_code error;
	std::filesystem::create_directories(path, error);
	if (error) {
		return(false);
	}

	return(std::filesystem::is_directory(path, error) && !error);
}

}


IOSPaths Build_IOS_Paths(char const * home)
{
	IOSPaths paths;

	if (home == nullptr || home[0] == '\0') {
		return(paths);
	}

	paths.Root = std::filesystem::path(home) / "Documents" / "OpenTS";
	paths.Data = paths.Root / "Data";
	paths.User = paths.Root / "User";
	return(paths);
}


bool Create_IOS_Paths(IOSPaths const & paths)
{
	if (!paths.Valid()) {
		return(false);
	}

	// Keep these separate and create Data first: common GameDirs intentionally fails closed
	// when a named DATADIR does not exist, while User is the writable player state root.
	bool const data = Ensure_Directory(paths.Data);
	bool const user = Ensure_Directory(paths.User);
	return(data && user);
}


std::vector<std::string> IOS_Command_Line_Arguments(IOSPaths const & paths)
{
	if (!paths.Valid()) {
		return(std::vector<std::string>());
	}

	return({
		std::string("-DATADIR=") + paths.Data.string(),
		std::string("-USERDIR=") + paths.User.string()
	});
}


std::filesystem::path IOS_Log_Directory(IOSPaths const & paths)
{
	return(paths.Valid() ? paths.User : std::filesystem::path());
}


std::filesystem::path IOS_Touch_Log_Directory(IOSPaths const & paths)
{
	return(paths.Valid() ? paths.User / "touchlog" : std::filesystem::path());
}
