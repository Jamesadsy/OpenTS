/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "savefileenum.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif


namespace
{


int Failures = 0;
int Checks = 0;
std::filesystem::path Root;


void Check(char const * name, bool condition)
{
	Checks++;
	if (condition) {
		return;
	}

	Failures++;
	std::printf("FAIL %s\n", name);
}


void Make_File(std::filesystem::path const & path)
{
	std::ofstream file(path, std::ios::binary);
	file << "synthetic save candidate";
}


void Set_Time(std::filesystem::path const & path, std::chrono::seconds offset)
{
	std::error_code error;
	auto const now = std::filesystem::file_time_type::clock::now();
	std::filesystem::last_write_time(path, now + offset, error);
	Check("synthetic timestamp is writable", !error);
}


bool Has_Name(std::vector<SaveFileRecord> const & records, char const * name)
{
	for (SaveFileRecord const & record : records) {
		if (record.Filename == name) {
			return(true);
		}
	}
	return(false);
}


void Test_Extension_And_Regular_File_Eligibility(void)
{
	std::filesystem::path const folder = Root / "eligibility";
	std::filesystem::create_directories(folder);
	Make_File(folder / "NEW.SAV");
	Make_File(folder / "mixed.sAv");
	Make_File(folder / "wrong.MIX");
	Make_File(folder / ".hidden.SAV");
	std::filesystem::create_directory(folder / "not-a-file.SAV");

#ifdef _WIN32
	Check("hidden attribute is writable", SetFileAttributesA((folder / ".hidden.SAV").string().c_str(), FILE_ATTRIBUTE_HIDDEN) != 0);
#endif

	Set_Time(folder / "NEW.SAV", std::chrono::seconds(20));
	Set_Time(folder / "mixed.sAv", std::chrono::seconds(10));

	std::vector<SaveFileRecord> const records = Enumerate_Save_Files(folder.string(), "sav");
	Check("requested extension matches", records.size() == 2);
	Check("extension matching is case insensitive", Has_Name(records, "mixed.sAv"));
	Check("wrong extension is rejected", !Has_Name(records, "wrong.MIX"));
	Check("hidden entries are rejected", !Has_Name(records, ".hidden.SAV"));
	Check("directories are not regular save files", !Has_Name(records, "not-a-file.SAV"));
	Check("filename is transported", Has_Name(records, "NEW.SAV"));
	for (SaveFileRecord const & record : records) {
		Check("portable timestamp is transported", record.ModifiedAt != std::chrono::system_clock::time_point::min());
	}
}


void Test_Newest_First_Ordering(void)
{
	std::filesystem::path const folder = Root / "ordering";
	std::filesystem::create_directories(folder);
	Make_File(folder / "older.SAV");
	Make_File(folder / "newer.SAV");
	Set_Time(folder / "older.SAV", std::chrono::seconds(-30));
	Set_Time(folder / "newer.SAV", std::chrono::seconds(30));

	std::vector<SaveFileRecord> const records = Enumerate_Save_Files(folder.string(), ".SAV");
	Check("newest-first has both records", records.size() == 2);
	Check("newest-first ordering", records.size() == 2 && records[0].Filename == "newer.SAV");
	Check("newest timestamp compares greater", records.size() == 2 && records[0].ModifiedAt > records[1].ModifiedAt);
}


void Test_Missing_And_Empty_Directories(void)
{
	std::filesystem::path const missing = Root / "missing";
	std::vector<SaveFileRecord> const missing_records = Enumerate_Save_Files(missing.string(), "SAV");
	Check("missing save directory means no saves", missing_records.empty());

	std::filesystem::path const empty = Root / "empty";
	std::filesystem::create_directory(empty);
	std::vector<SaveFileRecord> const empty_records = Enumerate_Save_Files(empty.string(), "SAV");
	Check("empty save directory means no saves", empty_records.empty());
}


bool Make_Root(void)
{
	std::error_code error;
	std::filesystem::path const base = std::filesystem::temp_directory_path(error);
	if (error) {
		return(false);
	}

	for (unsigned int attempt = 0; attempt < 100; attempt++) {
		std::filesystem::path candidate = base / ("opents-savebrowser-" + std::to_string(attempt));
		if (!std::filesystem::exists(candidate, error)) {
			std::filesystem::create_directory(candidate, error);
			if (!error) {
				Root = candidate;
				return(true);
			}
		}
		error.clear();
	}
	return(false);
}


}


int main(void)
{
	if (!Make_Root()) {
		std::printf("could not create the synthetic save-browser directory\n");
		return(1);
	}

	Test_Extension_And_Regular_File_Eligibility();
	Test_Newest_First_Ordering();
	Test_Missing_And_Empty_Directories();

	std::error_code error;
	std::filesystem::remove_all(Root, error);
	Check("synthetic directory cleanup", !error);
	std::printf("%d save-browser checks: %s\n", Checks, Failures == 0 ? "all passed" : "failures");
	return(Failures == 0 ? 0 : 1);
}
