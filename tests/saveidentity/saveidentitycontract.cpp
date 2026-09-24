/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Exercises manual save target selection using only synthetic files.

#include "saveidentity.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {

int Failures = 0;
std::filesystem::path Scratch;

void Check(bool condition, char const * name)
{
	std::printf("%-68s %s\n", name, condition ? "ok" : "FAILED");
	if (!condition) Failures++;
}

void Write_Selected_Save(std::filesystem::path const & directory, bool selected_is_save,
	std::string const & selected_filename, std::string const & new_slot_filename,
	std::string const & description, std::string const & stamp)
{
	std::filesystem::create_directories(directory);
	std::string const target = Save_Target_Filename(selected_is_save, selected_filename, new_slot_filename);
	std::ofstream file(directory / target, std::ios::binary | std::ios::trunc);
	file << stamp << '\n' << description << '\n';
}

std::string Read_Save(std::filesystem::path const & file_name)
{
	std::ifstream file(file_name, std::ios::binary);
	return(std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()));
}

std::size_t Save_Count(std::filesystem::path const & directory)
{
	std::size_t count = 0;
	for (auto const & file : std::filesystem::directory_iterator(directory)) {
		if (file.is_regular_file() && file.path().extension() == ".SAV") count++;
	}
	return(count);
}

}

int main(void)
{
	Scratch = std::filesystem::temp_directory_path() / "opents-save-identity-contract";
	std::error_code error;
	std::filesystem::remove_all(Scratch, error);
	std::filesystem::create_directories(Scratch);

	std::filesystem::path const saves = Scratch / "Saved Games";
	std::filesystem::path const base = saves / "Tiberian Sun";
	std::filesystem::path const firestorm = saves / "Firestorm";
	std::filesystem::create_directories(saves);
	{
		std::ofstream legacy(saves / "LEGACY.SAV", std::ios::binary);
		legacy << "preserve this legacy save";
	}

	Write_Selected_Save(base, false, "", "SAVE0001.SAV", "New game", "created");
	Check(Save_Count(base) == 1 && std::filesystem::exists(base / "SAVE0001.SAV"),
		"a new manual save creates one file in its empty slot");

	// A repeated action carries the selected row's filename and updates that exact file.
	for (int overwrite = 0; overwrite < 3; overwrite++) {
		Write_Selected_Save(base, true, "SAVE0001.SAV", "SAVE0002.SAV", "Renamed description",
			"overwrite " + std::to_string(overwrite));
	}
	Check(Save_Count(base) == 1 && Read_Save(base / "SAVE0001.SAV").find("overwrite 2") != std::string::npos
		&& Read_Save(base / "SAVE0001.SAV").find("Renamed description") != std::string::npos,
		"repeated overwrite refreshes the selected save's timestamp and description without duplicates");

	// Identical descriptions have no part in resolving which row is written.
	Write_Selected_Save(base, false, "", "SAVE0002.SAV", "Same description", "other file");
	std::string const first_before = Read_Save(base / "SAVE0001.SAV");
	Write_Selected_Save(base, true, "SAVE0002.SAV", "SAVE0003.SAV", "Same description", "selected file updated");
	Check(Read_Save(base / "SAVE0001.SAV") == first_before
		&& Read_Save(base / "SAVE0002.SAV").find("selected file updated") != std::string::npos,
		"duplicate descriptions cannot redirect overwrite away from the selected filename");

	Write_Selected_Save(firestorm, false, "", "SAVE0001.SAV", "Firestorm game", "Firestorm data");
	Check(Save_Count(base) == 2 && Save_Count(firestorm) == 1
		&& Read_Save(base / "SAVE0001.SAV").find("overwrite 2") != std::string::npos
		&& Read_Save(firestorm / "SAVE0001.SAV").find("Firestorm data") != std::string::npos,
		"Base TS and Firestorm manual save namespaces stay isolated");
	Check(Read_Save(saves / "LEGACY.SAV") == "preserve this legacy save",
		"existing legacy saves are preserved without migration or cleanup");

	std::filesystem::remove_all(Scratch, error);
	std::printf("\n%s\n", Failures == 0 ? "PASSED" : "FAILED");
	return(Failures == 0 ? 0 : 1);
}
