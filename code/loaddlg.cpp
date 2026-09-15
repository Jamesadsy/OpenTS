/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

/* $Header: /CounterStrike/LOADDLG.CPP 1     3/03/97 10:25a Joe_bostic $ */

#include "always.h"

#include "autosave.h"

#include "loaddlg.h"

#include "data.h"
#include "gamedirs.h"
#include "globals.h"
#include "houstype.h"
#include "init.h"
#include "language/language.h"
#include "saveload.h"
#include "savever.h"
#include "scenario.h"
#include "session.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <system_error>
#include <vector>


LoadOptionsClass::LoadOptionsClass(void) :
	Files(0),
	Style(NONE),
	Description(NULL),
	Callback(NULL),
	State(STATE_PENDING)
{
	Extension = "SAV";
	MinSpaceRequired = 2048;
	Files.Clear();
}


LoadOptionsClass::~LoadOptionsClass(void)
{
	Clear_List();
}


bool LoadOptionsClass::Load(void)
{
	Style = LOAD;
	Description = NULL;
	return(Dialog());
}


bool LoadOptionsClass::Save(char * description)
{
	Style = SAVE;
	Description = description;
	return(Dialog());
}


bool LoadOptionsClass::Delete(void)
{
	Style = WWDELETE;
	Description = NULL;
	return(Dialog());
}


static bool Saved_Game_Exists(char const * name)
{
	std::error_code error;
	return(std::filesystem::exists(Saved_Game_Name(name), error) && !error);
}


void LoadOptionsClass::Pick_Filename(char * name)
{
	do {
		sprintf(name, "SAVE%04lX.%3s", rand(), Extension);
	} while (Saved_Game_Exists(name));
}


void LoadOptionsClass::Clear_List(void)
{
	for (int i = 0; i < Files.Count(); i++) {
		delete Files[i];
	}
	Files.Clear();
}


void LoadOptionsClass::Build_List(void)
{
	FileEntryClass * fdata = NULL;

	/*
	 * Keep the accepted H1 synthetic SAVE row and portable enumeration contract. The native
	 * adapter receives a value snapshot of this list for rendering.
	 */
	Clear_List();
	if (Style == SAVE) {
		fdata = new FileEntryClass;
		strcpy(fdata->Descr, Fetch_String(TXT_EMPTY_SLOT));
		if (PlayerPtr != NULL) {
			fdata->Scenario = Scen->Scenario;
			fdata->House = Scen->PlayerHouse;
			fdata->Num = Scen->Campaign;
			strcpy(fdata->PlayerName, PlayerPtr->Class->GivenName);
		} else {
			fdata->Scenario = 0;
			fdata->House = (HousesType)Session.House;
			fdata->Num = -1;
			strcpy(fdata->PlayerName, Session.Handle);
		}
		fdata->DateTime = std::chrono::system_clock::now();
		fdata->Type = Session.Type;
		fdata->Valid = false;
		Files.Add(fdata);
	}

	/* Saved_Game_Name remains the selected save-root authority. */
	std::vector<SaveFileRecord> const found = Enumerate_Save_Files(Saved_Game_Name(""), Extension);
	std::size_t const scan_limit = Scan_Limit();
	std::size_t const records_to_read = std::min(found.size(), scan_limit);

	/* Scan_Limit is applied after ordering and before any save-header read. */
	for (std::size_t index = 0; index < records_to_read; index++) {
		SaveFileRecord const & record = found[index];
		fdata = new FileEntryClass;
		if (Read_File(fdata, &record)) {
			Files.Add(fdata);
		} else {
			delete fdata;
		}
	}

	if (Files.Count() > 1) {
		qsort((void *)(&Files[0]), Files.Count(), sizeof(class FileEntryClass *), LoadOptionsClass::Compare);
	}
}


bool LoadOptionsClass::Files_Present(void)
{
	std::vector<SaveFileRecord> const found = Enumerate_Save_Files(Saved_Game_Name(""), Extension);
	for (SaveFileRecord const & record : found) {
		FileEntryClass entry;
		if (Read_File(&entry, &record) == true) {
			return(true);
		}
	}
	return(false);
}


int LoadOptionsClass::Compare(const void * p1, const void * p2)
{
	FileEntryClass * fe1 = *((FileEntryClass **)p1);
	FileEntryClass * fe2 = *((FileEntryClass **)p2);

	if (fe1->DateTime > fe2->DateTime) {
		return(-1);
	}
	if (fe1->DateTime < fe2->DateTime) {
		return(1);
	}
	return(0);
}


int LoadOptionsClass::Save_Confirmation(void) const
{
	return(TXT_NONE);
}


bool LoadOptionsClass::Delete_File(const char * file_name)
{
	std::string const path = Saved_Game_Name(file_name);
	std::error_code error;
	if (!std::filesystem::is_regular_file(path, error) || error) {
		return(false);
	}
	return(std::filesystem::remove(path, error) && !error);
}


bool LoadOptionsClass::Read_File(FileEntryClass * fdata, SaveFileRecord const * record)
{
	if (fdata == NULL || record == NULL) {
		return(false);
	}

	SaveVersionInfo savever;
	if (!Get_Savefile_Info(record->Filename.c_str(), &savever)) {
		return(false);
	}

	if (savever.Get_Internal_Version() != ExpectedGameVersion) {
		return(false);
	}

	snprintf(fdata->Descr, sizeof(fdata->Descr), "%s", savever.Get_Scenario_Description());
	fdata->Valid = true;
	fdata->Scenario = savever.Get_Scenario_Number();
	fdata->Num = savever.Get_Campaign_Number();
	fdata->Type = (GameType)savever.Get_Game_Type();
	std::snprintf(fdata->Filename, sizeof(fdata->Filename), "%s", record->Filename.c_str());
	strcpy(fdata->PlayerName, savever.Get_Player_House());
	fdata->DateTime = record->ModifiedAt;
	return(true);
}


MultiplayerLoadOptionsClass::MultiplayerLoadOptionsClass(void)
{
	Extension = "NET";
	Picked[0] = '\0';
}


bool MultiplayerLoadOptionsClass::Load_File(const char * file_name)
{
	std::snprintf(Picked, sizeof(Picked), "%s", file_name);
	return(true);
}


bool MultiplayerLoadOptionsClass::Read_File(FileEntryClass * entry, SaveFileRecord const * record)
{
	if (entry == NULL || record == NULL || Multiplayer_Save_Slot(record->Filename.c_str()) < 0) {
		return(false);
	}
	return(LoadOptionsClass::Read_File(entry, record) && entry->Type == Session.Type);
}
