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
#include "ui/savebrowsernavigation.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace {

int Failures = 0;
std::filesystem::path Scratch;

struct BrowserEntryType
{
	std::string Filename;
	std::string Description;
	bool Valid = false;
};

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

void Test_Manual_Save_Recall(void)
{
	using ProductType = ManualSaveIdentityClass::ProductType;
	ProductType const base_product = ProductType::TiberianSun;
	ProductType const firestorm_product = ProductType::Firestorm;
	std::vector<BrowserEntryType> entries = {
		{"", "[EMPTY SLOT]", false},
		{"SAVE0001.SAV", "Duplicate description", true},
		{"SAVE0002.SAV", "Duplicate description", true},
	};

	ManualSaveIdentityClass failed_load;
	failed_load.Record_Load(base_product, "BROKEN.SAV", false);
	Check(failed_load.File_Name(base_product).empty(),
		"a failed manual Load does not adopt its target file identity");

	ManualSaveIdentityClass identity;
	identity.Record_Load(base_product, "SAVE0002.SAV", true);
	Check(identity.File_Name(base_product) == "SAVE0002.SAV",
		"a successful manual Load stores the exact file identity");
	Check(Initial_Save_Identity_Row(identity.File_Name(base_product), entries) == 2,
		"Save preselects the exact current file despite duplicate descriptions");

	identity.Record_Load(base_product, "BROKEN.SAV", false);
	Check(identity.File_Name(base_product) == "SAVE0002.SAV",
		"a failed Load leaves the prior current file identity unchanged");

	std::string const overwrite = Save_Target_Filename(true, identity.File_Name(base_product), "SAVE0003.SAV");
	identity.Record_Save(base_product, overwrite, true);
	Check(identity.File_Name(base_product) == "SAVE0002.SAV",
		"a successful overwrite keeps the same file current");

	identity.Record_Save(base_product, "SAVE0003.SAV", false);
	Check(identity.File_Name(base_product) == "SAVE0002.SAV",
		"a failed Save does not adopt its target file identity");

	ManualSaveIdentityClass fresh;
	Check(Initial_Save_Identity_Row(fresh.File_Name(base_product), entries) == 0,
		"a fresh session opens Save on the empty slot");
	std::string const created = Save_Target_Filename(false, "", "SAVE0003.SAV");
	fresh.Record_Save(base_product, created, true);
	entries.push_back({created, "New save", true});
	Check(fresh.File_Name(base_product) == created
		&& Initial_Save_Identity_Row(fresh.File_Name(base_product), entries) == 3,
		"a successful new Save adopts and preselects its created file");

	identity.Record_Save(firestorm_product, "SAVE0001.SAV", true);
	Check(identity.File_Name(base_product) == "SAVE0002.SAV"
		&& identity.File_Name(firestorm_product) == "SAVE0001.SAV",
		"Base TS and Firestorm current-file identities remain isolated");
	identity.Clear();
	Check(identity.File_Name(base_product).empty() && identity.File_Name(firestorm_product).empty(),
		"starting a fresh session clears both product identities");
}


void Test_Save_Browser_Navigation(void)
{
	std::vector<BrowserEntryType> entries = {
		{"", "Suggested new game", false},
		{"SAVE0001.SAV", "First save", true},
		{"SAVE0002.SAV", "Last save", true},
	};

	Check(Save_Browser_Next_Row(1, entries.size(), -1) == 0
		&& Save_Browser_Next_Row(1, entries.size(), 1) == 2,
		"D-pad Up and Down move the highlighted row one step");
	Check(Save_Browser_Next_Row(0, entries.size(), -1) == 2
		&& Save_Browser_Next_Row(2, entries.size(), 1) == 0,
		"D-pad navigation wraps between the first and last rows");
	Check(Save_Browser_Next_Row(0, entries.size(), 1) == 1
		&& Save_Browser_Next_Row(1, entries.size(), -1) == 0,
		"the Save empty slot participates in row cycling");
	Check(Save_Browser_Next_Row(-1, 0, 1) == -1,
		"an empty list has no selectable row");

	Check(Save_Browser_Description(2, entries, "Suggested new game") == "Last save"
		&& Save_Browser_Description(0, entries, "Suggested new game") == "Suggested new game",
		"Save description follows the highlighted file or empty slot");

	int scrolled = -1;
	Save_Browser_Scroll_Selected_Row(2, entries.size(), [&scrolled](int row) { scrolled = row; });
	Check(scrolled == 2, "selection changes request scrolling the selected row into view");
	Save_Browser_Scroll_Selected_Row(3, entries.size(), [&scrolled](int row) { scrolled = row; });
	Check(scrolled == 2, "scrolling ignores a selection outside the list");
}

}

int main(void)
{
	Test_Manual_Save_Recall();
	Test_Save_Browser_Navigation();

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
