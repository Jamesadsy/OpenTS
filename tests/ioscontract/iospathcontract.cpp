/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Synthetic, no-owner-data proof for the iOS host path policy.

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <string>
#include <vector>

#include "iospaths.h"

namespace
{

int Failures = 0;

void Check(bool condition, char const * what)
{
	std::printf("%-66s %s\n", what, condition ? "ok" : "FAILED");
	if (!condition) Failures++;
}

bool Is_Child(std::filesystem::path const & child, std::filesystem::path const & parent)
{
	std::filesystem::path const relative = child.lexically_relative(parent);
	return(!relative.empty() && relative != "." && relative.begin()->string() != "..");
}

void Test_Synthetic_Home(void)
{
	std::filesystem::path const root = std::filesystem::temp_directory_path()
		/ ("opents-ios-path-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
	std::filesystem::path const home = root / "synthetic-container";
	IOSPaths const paths = Build_IOS_Paths(home.string().c_str());

	Check(paths.Valid(), "synthetic HOME produces a valid iOS path set");
	Check(paths.Root == home / "Documents" / "OpenTS", "Root is HOME/Documents/OpenTS");
	Check(paths.Data == home / "Documents" / "OpenTS" / "Data", "Data is HOME/Documents/OpenTS/Data");
	Check(paths.User == home / "Documents" / "OpenTS" / "User", "User is HOME/Documents/OpenTS/User");
	Check(paths.Data != paths.User, "Data and User are distinct");
	Check(Is_Child(paths.Data, home) && Is_Child(paths.User, home),
		"Data and User remain runtime-derived beneath HOME");
	Check(paths.Data.string().find("Documents/OpenTS/Data") != std::string::npos,
		"Data has no baked container UUID or alternate root");
	Check(IOS_Log_Directory(paths) == paths.User, "host log directory resolves to User");
	Check(IOS_Touch_Log_Directory(paths) == paths.User / "touchlog",
		"touch diagnostic directory resolves beneath User");

	std::vector<std::string> const arguments = IOS_Command_Line_Arguments(paths);
	Check(arguments.size() == 2, "exactly two iOS directory arguments are produced");
	Check(arguments.size() == 2
		&& arguments[0] == std::string("-DATADIR=") + paths.Data.string(),
		"DATADIR argument names Data");
	Check(arguments.size() == 2
		&& arguments[1] == std::string("-USERDIR=") + paths.User.string(),
		"USERDIR argument names User");

	Check(Create_IOS_Paths(paths), "required Data and User directories are created");
	std::error_code error;
	Check(std::filesystem::is_directory(paths.Data, error) && !error, "created Data is a directory");
	error.clear();
	Check(std::filesystem::is_directory(paths.User, error) && !error, "created User is a directory");

	std::filesystem::remove_all(root, error);
}


void Test_Invalid_Home(void)
{
	IOSPaths const paths = Build_IOS_Paths(nullptr);
	Check(!paths.Valid(), "missing HOME does not fabricate an iOS path");
	Check(IOS_Command_Line_Arguments(paths).empty(), "missing HOME produces no directory arguments");
	Check(IOS_Log_Directory(paths).empty() && IOS_Touch_Log_Directory(paths).empty(),
		"missing HOME produces no log or touch path");
	Check(!Create_IOS_Paths(paths), "missing HOME does not create directories");
}

}


int main(void)
{
	Test_Synthetic_Home();
	Test_Invalid_Home();
	std::printf("%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
