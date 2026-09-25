/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Exercises scenario-based autosave naming, overwrite behavior and interval timing.

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>

#include "autosave.h"

namespace {

using KindType = AutosaveClass::KindType;
using ProductType = AutosaveClass::ProductType;

int Failures = 0;
std::filesystem::path Scratch;


void Check(bool condition, char const * what)
{
	std::printf("%-72s %s\n", what, condition ? "ok" : "FAILED");
	if (!condition) Failures++;
}


void Write_Save(std::filesystem::path const & path, std::string const & contents)
{
	std::filesystem::create_directories(path.parent_path());
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	file << contents;
}


std::string Read_Save(std::filesystem::path const & path)
{
	std::ifstream file(path, std::ios::binary);
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


void Test_Stable_Names(void)
{
	std::string const scenario = "MAPS\\SCG01EA.INI";
	std::string const campaign = AutosaveClass::File_Name(KindType::Campaign, scenario);
	std::string const normalized = AutosaveClass::File_Name(KindType::Campaign, "maps/SCG01EA.ini");
	std::string const other_campaign = AutosaveClass::File_Name(KindType::Campaign, "SCG02EA.INI");
	std::string const skirmish = AutosaveClass::File_Name(KindType::Skirmish, scenario);

	Check(campaign.rfind("AUTOSAVE_", 0) == 0 && campaign.size() == 53
		&& campaign.substr(campaign.size() - 4) == ".SAV",
		"campaign autosave names are filesystem-safe scenario digests");
	Check(campaign == normalized, "scenario path case and separator changes retain the same identity");
	Check(campaign != other_campaign, "different scenario identities produce different filenames");
	Check(skirmish.rfind("AUTOSAVE_SKIRMISH_", 0) == 0 && skirmish != campaign,
		"skirmish autosaves use a separate stable scenario identity");

	AutosaveClass first;
	first.Seed_Slots(ProductType::TiberianSun, 4, 3);
	AutosaveClass reconstructed;
	reconstructed.Seed_Slots(ProductType::TiberianSun, 1, 0);
	std::string const restored_scenario = scenario;
	Check(campaign == AutosaveClass::File_Name(KindType::Campaign, restored_scenario),
		"a reconstructed same mission resolves to the same autosave filename");
	Check(first.Campaign_Slot(ProductType::TiberianSun) != reconstructed.Campaign_Slot(ProductType::TiberianSun)
		&& campaign == AutosaveClass::File_Name(KindType::Campaign, scenario),
		"legacy slot seeds do not change a mission autosave identity");
}


void Test_Overwrite_And_Legacy_Preservation(void)
{
	Scratch = std::filesystem::temp_directory_path() / "opents-autosave-identity-contract";
	std::error_code error;
	std::filesystem::remove_all(Scratch, error);
	std::filesystem::path const base = Scratch / "Tiberian Sun";
	std::filesystem::path const firestorm = Scratch / "Firestorm";
	std::filesystem::create_directories(base);
	std::filesystem::create_directories(firestorm);

	for (int slot = 1; slot <= AutosaveClass::SLOT_COUNT; slot++) {
		Write_Save(base / ("AUTOSAVE" + std::to_string(slot) + ".SAV"), "legacy campaign " + std::to_string(slot));
		Write_Save(base / ("AUTOSAVE_SKIRMISH" + std::to_string(slot) + ".SAV"), "legacy skirmish " + std::to_string(slot));
	}
	std::size_t const legacy_count = Save_Count(base);

	std::string const mission = "SCG01EA.INI";
	std::string const campaign_name = AutosaveClass::File_Name(KindType::Campaign, mission);
	std::filesystem::path const campaign_path = base / campaign_name;
	Write_Save(campaign_path, "first autosave");
	std::size_t const first_count = Save_Count(base);
	Write_Save(base / AutosaveClass::File_Name(KindType::Campaign, mission), "second autosave");
	Check(first_count == legacy_count + 1 && Save_Count(base) == first_count
		&& Read_Save(campaign_path) == "second autosave",
		"repeated same-mission autosaves overwrite one file instead of adding entries");

	std::string const other_name = AutosaveClass::File_Name(KindType::Campaign, "SCG02EA.INI");
	Write_Save(base / other_name, "other mission");
	Check(other_name != campaign_name && Save_Count(base) == first_count + 1,
		"a different mission writes a distinct stable autosave file");

	std::string const skirmish_name = AutosaveClass::File_Name(KindType::Skirmish, mission);
	std::filesystem::path const skirmish_path = base / skirmish_name;
	Write_Save(skirmish_path, "first skirmish autosave");
	Write_Save(base / AutosaveClass::File_Name(KindType::Skirmish, mission), "second skirmish autosave");
	Check(Save_Count(base) == first_count + 2 && Read_Save(skirmish_path) == "second skirmish autosave",
		"repeated skirmish autosaves retain one stable file identity");

	Write_Save(firestorm / campaign_name, "Firestorm autosave");
	Check(Read_Save(campaign_path) == "second autosave"
		&& Read_Save(firestorm / campaign_name) == "Firestorm autosave",
		"Base TS and Firestorm keep identical autosave names in separate save directories");

	bool legacy_preserved = Save_Count(base) == legacy_count + 3;
	for (int slot = 1; slot <= AutosaveClass::SLOT_COUNT; slot++) {
		legacy_preserved = legacy_preserved
			&& Read_Save(base / ("AUTOSAVE" + std::to_string(slot) + ".SAV")) == "legacy campaign " + std::to_string(slot)
			&& Read_Save(base / ("AUTOSAVE_SKIRMISH" + std::to_string(slot) + ".SAV")) == "legacy skirmish " + std::to_string(slot);
	}
	Check(legacy_preserved, "legacy AUTOSAVE1..5 campaign and skirmish files remain untouched");

	std::filesystem::remove_all(Scratch, error);
}


void Test_Interval_Timing(void)
{
	AutosaveClass autosave;

	autosave.Schedule(100);
	Check(!autosave.Is_Due(100000), "no interval means no save ever falls due");

	autosave.Set_Interval(300);
	autosave.Schedule(100);
	Check(!autosave.Is_Due(399) && autosave.Is_Due(400) && autosave.Is_Due(401),
		"a save falls due one interval after it was scheduled");

	autosave.Arm();
	Check(!autosave.Is_Due(400), "an armed request is not due again");
	Check(autosave.Take_Armed() && !autosave.Take_Armed(), "an armed request is taken once");

	autosave.Arm();
	autosave.Schedule(400);
	Check(!autosave.Take_Armed(), "a completed save drops an armed request");
	Check(!autosave.Is_Due(699) && autosave.Is_Due(700),
		"a completed save pushes the next one a full interval out");

	autosave.Set_Interval(0);
	autosave.Schedule(700);
	Check(!autosave.Is_Due(100000), "turning the interval off stops the saves");

	autosave.Set_Interval(-5);
	Check(autosave.Interval() == 0, "an interval below zero is off");

	autosave.Arm();
	autosave.Arm();
	Check(autosave.Take_Armed() && !autosave.Take_Armed(),
		"explicit requests coalesce with the timed interval disabled");
	Check(!autosave.Is_Due(100000), "an explicit request does not enable timed saves");
}


void Test_Multiplayer_And_Quick_Save_Names(void)
{
	Check(Multiplayer_Save_File_Name(0) == "SVGM_000.NET", "the first multiplayer save is SVGM_000.NET");
	Check(Multiplayer_Save_File_Name(MULTIPLAYER_SAVE_SLOTS - 1) == "SVGM_999.NET", "the last multiplayer save is SVGM_999.NET");
	Check(Multiplayer_Save_Slot("SVGM_007.NET") == 7, "a numbered name yields its slot");
	Check(Multiplayer_Save_Slot("svgm_007.net") == 7, "the pattern is matched in any case");
	Check(Multiplayer_Save_Slot("SAVEGAME.NET") == -1, "the old fixed name is not a slot");
	Check(Multiplayer_Save_Slot("SVGM_00A.NET") == -1, "a non-digit is refused");
	Check(Multiplayer_Save_Slot("SVGM_0007.NET") == -1, "four digits are refused");
	Check(Multiplayer_Save_Slot("SVGM_007.SAV") == -1, "another extension is refused");
	Check(Multiplayer_Save_Slot("") == -1 && Multiplayer_Save_Slot(nullptr) == -1,
		"an empty or null name is refused");
	Check(Quick_Save_File_Name(KindType::Campaign) == "QUICKSAVE.SAV",
		"the campaign quick save has one fixed name");
	Check(Quick_Save_File_Name(KindType::Skirmish) == "QUICKSAVE_SKIRMISH.SAV",
		"the skirmish quick save keeps a file of its own");
}

}


int main(void)
{
	Test_Stable_Names();
	Test_Overwrite_And_Legacy_Preservation();
	Test_Interval_Timing();
	Test_Multiplayer_And_Quick_Save_Names();

	std::printf("\n%s\n", Failures == 0 ? "PASSED" : "FAILED");
	return(Failures == 0 ? 0 : 1);
}
