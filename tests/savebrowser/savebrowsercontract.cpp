/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "savebrowser.h"
#include "savefileenum.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
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


FileEntryClass Make_Entry(char const * filename, char const * description, bool valid,
	GameType type = GAME_NORMAL)
{
	FileEntryClass entry;
	std::snprintf(entry.Filename, sizeof(entry.Filename), "%s", filename);
	std::snprintf(entry.Descr, sizeof(entry.Descr), "%s", description);
	std::snprintf(entry.PlayerName, sizeof(entry.PlayerName), "%s", "Synthetic Player");
	entry.Scenario = 7;
	entry.House = HOUSE_FIRST;
	entry.Num = 42;
	entry.Valid = valid;
	entry.Type = type;
	entry.DateTime = std::chrono::system_clock::now();
	return(entry);
}


class RecordingSaveBrowserService final : public SaveBrowserService
{
	public:
		bool LoadResult = true;
		bool SaveResult = true;
		bool DeleteResult = true;
		bool ExistsResult = false;
		std::string PickedFilename = "SAVE0001.SAV";
		std::vector<FileEntryClass> RefreshedEntries;
		int LoadCount = 0;
		int SaveCount = 0;
		int DeleteCount = 0;
		mutable int ExistsCount = 0;
		int PickCount = 0;
		int RefreshCount = 0;
		int ServiceCount = 0;
		std::string LastFilename;
		std::string LastDescription;

		bool Load_File(std::string const & file_name) override
		{
			LoadCount++;
			LastFilename = file_name;
			return(LoadResult);
		}

		bool Save_File(std::string const & file_name, std::string const & description) override
		{
			SaveCount++;
			LastFilename = file_name;
			LastDescription = description;
			return(SaveResult);
		}

		bool Delete_File(std::string const & file_name) override
		{
			DeleteCount++;
			LastFilename = file_name;
			return(DeleteResult);
		}

		bool Save_Exists(std::string const &) const override
		{
			ExistsCount++;
			return(ExistsResult);
		}

		std::string Pick_Filename(void) override
		{
			PickCount++;
			return(PickedFilename);
		}

		std::vector<FileEntryClass> Refresh(void) override
		{
			RefreshCount++;
			return(RefreshedEntries);
		}

		void Service(void) override
		{
			ServiceCount++;
		}
};


void Test_Presenter_Mode_Selection_And_Metadata(void)
{
	RecordingSaveBrowserService service;
	std::vector<FileEntryClass> load_entries = {
		Make_Entry("invalid.SAV", "invalid", false),
		Make_Entry("valid.SAV", "stored", true, GAME_SKIRMISH),
	};
	SaveBrowserPresenter load(SaveBrowserMode::LOAD, load_entries, "", service);
	Check("presenter LOAD mode", load.Mode() == SaveBrowserMode::LOAD);
	Check("LOAD initial selection is first valid entry", load.Selected_Index() == 1);
	Check("LOAD entry metadata is retained", load.Selected_Entry() != nullptr &&
		load.Selected_Entry()->Type == GAME_SKIRMISH && load.Selected_Entry()->Scenario == 7);

	std::vector<FileEntryClass> save_entries = {
		Make_Entry("", "[EMPTY SLOT]", false),
		Make_Entry("existing.SAV", "stored description", true),
	};
	SaveBrowserPresenter save(SaveBrowserMode::SAVE, save_entries, "suggested description", service);
	Check("presenter SAVE mode", save.Mode() == SaveBrowserMode::SAVE);
	Check("SAVE synthetic row is first selection", save.Selected_Index() == 0 &&
		!save.Selected_Entry()->Valid);
	Check("SAVE synthetic row stages initial description", save.Description() == "suggested description");
	Check("SAVE synthetic row is actionable with description", save.Action_Valid());
	Check("SAVE existing selection changes description", save.Select(1) &&
		save.Description() == "stored description");
	Check("SAVE selection returns to synthetic description", save.Select(0) &&
		save.Description() == "suggested description");

	SaveBrowserPresenter remove(SaveBrowserMode::DELETE_MODE, save_entries, "", service);
	Check("presenter DELETE mode", remove.Mode() == SaveBrowserMode::DELETE_MODE);
}


void Test_Presenter_Description_Validation(void)
{
	RecordingSaveBrowserService service;
	SaveBrowserPresenter presenter(SaveBrowserMode::SAVE,
		{Make_Entry("", "[EMPTY SLOT]", false)}, "", service);
	Check("empty SAVE description is invalid", !presenter.Action_Valid());
	Check("empty SAVE description is rejected", presenter.Accept() == SaveBrowserResult::PENDING &&
		presenter.Failure() == SaveBrowserFailure::INVALID_DESCRIPTION && service.SaveCount == 0);

	Check("description above accepted bound is rejected",
		!presenter.Set_Description(std::string(SaveBrowserPresenter::DESCRIPTION_LIMIT + 1, 'x')));
	Check("overlong description remains invalid", !presenter.Action_Valid() &&
		presenter.Failure() == SaveBrowserFailure::INVALID_DESCRIPTION);
	std::string const bounded(SaveBrowserPresenter::DESCRIPTION_LIMIT, 'x');
	Check("accepted description bound is writable", presenter.Set_Description(bounded));
	Check("accepted description bound saves", presenter.Accept() == SaveBrowserResult::ACCEPTED &&
		service.SaveCount == 1 && service.LastDescription.size() == SaveBrowserPresenter::DESCRIPTION_LIMIT);
}


void Test_Presenter_Load_Accept_And_Failure(void)
{
	RecordingSaveBrowserService service;
	SaveBrowserPresenter loaded(SaveBrowserMode::LOAD,
		{Make_Entry("load.SAV", "load me", true)}, "", service);
	Check("LOAD accept calls service once", loaded.Accept() == SaveBrowserResult::ACCEPTED &&
		service.LoadCount == 1 && service.LastFilename == "load.SAV");
	int const calls_after_load = service.LoadCount;
	Check("completed LOAD cannot run twice", loaded.Accept() == SaveBrowserResult::ACCEPTED &&
		service.LoadCount == calls_after_load);

	RecordingSaveBrowserService failed_service;
	failed_service.LoadResult = false;
	SaveBrowserPresenter failed(SaveBrowserMode::LOAD,
		{Make_Entry("load.SAV", "load me", true)}, "", failed_service);
	Check("LOAD failure keeps browser pending", failed.Accept() == SaveBrowserResult::PENDING &&
		failed.Result() == SaveBrowserResult::PENDING && failed.Failure() == SaveBrowserFailure::LOAD &&
		failed_service.LoadCount == 1);
}


void Test_Presenter_Save_New_Overwrite_And_Failure(void)
{
	RecordingSaveBrowserService new_service;
	new_service.PickedFilename = "new.SAV";
	SaveBrowserPresenter new_save(SaveBrowserMode::SAVE,
		{Make_Entry("", "[EMPTY SLOT]", false)}, "new description", new_service);
	Check("SAVE new-slot accept path", new_save.Accept() == SaveBrowserResult::ACCEPTED &&
		new_service.PickCount == 1 && new_service.SaveCount == 1 &&
		new_service.LastFilename == "new.SAV");

	RecordingSaveBrowserService overwrite_service;
	overwrite_service.ExistsResult = true;
	std::vector<FileEntryClass> overwrite_entries = {Make_Entry("old.SAV", "old", true)};
	SaveBrowserPresenter overwrite(SaveBrowserMode::SAVE, overwrite_entries, "", overwrite_service);
	overwrite.Set_Description("replacement");
	Check("overwrite requires semantic confirmation", overwrite.Accept() == SaveBrowserResult::PENDING &&
		overwrite.Pending_Confirmation() == SaveBrowserConfirmation::OVERWRITE &&
		overwrite_service.SaveCount == 0);
	Check("overwrite reject performs no save", overwrite.Reject() == SaveBrowserResult::PENDING &&
		overwrite.Pending_Confirmation() == SaveBrowserConfirmation::NONE &&
		overwrite_service.SaveCount == 0 && std::strcmp(overwrite.Entries()[0].Filename, "old.SAV") == 0);
	overwrite.Accept();
	Check("overwrite confirm performs save once", overwrite.Confirm() == SaveBrowserResult::ACCEPTED &&
		overwrite_service.SaveCount == 1);

	RecordingSaveBrowserService failed_service;
	failed_service.SaveResult = false;
	SaveBrowserPresenter failed_save(SaveBrowserMode::SAVE,
		{Make_Entry("new.SAV", "old", true)}, "", failed_service);
	failed_save.Set_Description("replacement");
	Check("save failure keeps browser pending", failed_save.Accept() == SaveBrowserResult::PENDING &&
		failed_save.Failure() == SaveBrowserFailure::SAVE && failed_service.SaveCount == 1);
}


void Test_Presenter_Delete_Confirmation_Refresh_And_Failure(void)
{
	RecordingSaveBrowserService service;
	std::vector<FileEntryClass> entries = {
		Make_Entry("first.SAV", "first", true),
		Make_Entry("second.SAV", "second", true),
	};
	service.RefreshedEntries = {entries[1]};
	SaveBrowserPresenter presenter(SaveBrowserMode::DELETE_MODE, entries, "", service);
	Check("DELETE requires semantic confirmation", presenter.Accept() == SaveBrowserResult::PENDING &&
		presenter.Pending_Confirmation() == SaveBrowserConfirmation::DELETE_FILE &&
		service.DeleteCount == 0);
	Check("delete reject leaves model unchanged", presenter.Reject() == SaveBrowserResult::PENDING &&
		service.DeleteCount == 0 && presenter.Entries().size() == 2 &&
		std::strcmp(presenter.Entries()[0].Filename, "first.SAV") == 0);

	presenter.Accept();
	Check("successful delete refreshes model", presenter.Confirm() == SaveBrowserResult::PENDING &&
		service.DeleteCount == 1 && service.RefreshCount == 1 && presenter.Entries().size() == 1 &&
		std::strcmp(presenter.Entries()[0].Filename, "second.SAV") == 0 && presenter.Selected_Index() == 0);

	RecordingSaveBrowserService failed_service;
	failed_service.DeleteResult = false;
	SaveBrowserPresenter failed(SaveBrowserMode::DELETE_MODE, entries, "", failed_service);
	failed.Accept();
	Check("failed delete remains pending and truthful", failed.Confirm() == SaveBrowserResult::PENDING &&
		failed.Failure() == SaveBrowserFailure::DELETE_FILE && failed.Entries().size() == 2 &&
		std::strcmp(failed.Entries()[0].Filename, "first.SAV") == 0 && failed_service.RefreshCount == 0);

	RecordingSaveBrowserService final_service;
	final_service.RefreshedEntries.clear();
	SaveBrowserPresenter final_row(SaveBrowserMode::DELETE_MODE,
		{Make_Entry("last.SAV", "last", true)}, "", final_service);
	final_row.Accept();
	Check("final delete leaves valid empty state", final_row.Confirm() == SaveBrowserResult::ACCEPTED &&
		final_row.Entries().empty() && !final_row.Has_Selection());
}


void Test_Presenter_Cancel_Service_And_H1_Metadata(void)
{
	RecordingSaveBrowserService service;
	FileEntryClass network_entry = Make_Entry("NET0001.NET", "network", true, GAME_INTERNET);
	SaveBrowserPresenter cancelled(SaveBrowserMode::LOAD, {network_entry}, "", service);
	Check("cancel remains cancel", cancelled.Cancel() == SaveBrowserResult::CANCELLED &&
		cancelled.Result() == SaveBrowserResult::CANCELLED && service.LoadCount == 0);
	Check("cancel cannot mutate later", cancelled.Accept() == SaveBrowserResult::CANCELLED && service.LoadCount == 0);

	RecordingSaveBrowserService service_seam;
	SaveBrowserPresenter serviced(SaveBrowserMode::LOAD, {network_entry}, "", service_seam);
	serviced.Service();
	Check("deterministic service seam is callable", service_seam.ServiceCount == 1 &&
		serviced.Result() == SaveBrowserResult::PENDING);
	Check("multiplayer/session metadata survives presenter split",
		serviced.Entries()[0].Type == GAME_INTERNET && serviced.Entries()[0].House == HOUSE_FIRST &&
		serviced.Entries()[0].Num == 42 && serviced.Entries()[0].PlayerName[0] != '\0');
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
	Test_Presenter_Mode_Selection_And_Metadata();
	Test_Presenter_Description_Validation();
	Test_Presenter_Load_Accept_And_Failure();
	Test_Presenter_Save_New_Overwrite_And_Failure();
	Test_Presenter_Delete_Confirmation_Refresh_And_Failure();
	Test_Presenter_Cancel_Service_And_H1_Metadata();

	std::error_code error;
	std::filesystem::remove_all(Root, error);
	Check("synthetic directory cleanup", !error);
	std::printf("%d save-browser checks: %s\n", Checks, Failures == 0 ? "all passed" : "failures");
	return(Failures == 0 ? 0 : 1);
}
