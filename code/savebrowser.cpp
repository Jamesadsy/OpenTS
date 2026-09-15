/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "savebrowser.h"

#include <utility>


SaveBrowserPresenter::SaveBrowserPresenter(SaveBrowserMode mode,
	std::vector<FileEntryClass> entries, std::string initial_description,
	SaveBrowserService & service) :
	ModeValue(mode),
	EntriesValue(std::move(entries)),
	InitialDescriptionValue(Bounded_Description(initial_description)),
	DescriptionValue(mode == SaveBrowserMode::SAVE ? InitialDescriptionValue : std::string()),
	ServiceValue(&service)
{
	if (ModeValue == SaveBrowserMode::LOAD) {
		for (std::size_t index = 0; index < EntriesValue.size(); index++) {
			if (EntriesValue[index].Valid) {
				SelectedIndex = static_cast<int>(index);
				break;
			}
		}
	} else if (!EntriesValue.empty()) {
		// SAVE and DELETE both open on the first row, matching the retained Windows view.
		SelectedIndex = 0;
	}
}


std::string SaveBrowserPresenter::Bounded_Description(std::string const & description)
{
	return(description.substr(0, DESCRIPTION_LIMIT));
}


void SaveBrowserPresenter::Set_Failure(SaveBrowserFailure failure)
{
	FailureValue = failure;
}


FileEntryClass const * SaveBrowserPresenter::Selected_Entry(void) const
{
	if (!Has_Selection() || static_cast<std::size_t>(SelectedIndex) >= EntriesValue.size()) {
		return(nullptr);
	}
	return(&EntriesValue[static_cast<std::size_t>(SelectedIndex)]);
}


bool SaveBrowserPresenter::Set_Description(std::string const & description)
{
	if (ModeValue != SaveBrowserMode::SAVE || description.size() > DESCRIPTION_LIMIT) {
		Set_Failure(SaveBrowserFailure::INVALID_DESCRIPTION);
		return(false);
	}

	DescriptionValue = description;
	Set_Failure(SaveBrowserFailure::NONE);
	return(true);
}


bool SaveBrowserPresenter::Action_Valid(void) const
{
	FileEntryClass const * entry = Selected_Entry();
	if (entry == nullptr) {
		return(false);
	}

	if (ModeValue == SaveBrowserMode::SAVE) {
		return(!DescriptionValue.empty() && DescriptionValue.size() <= DESCRIPTION_LIMIT);
	}

	return(entry->Valid);
}


bool SaveBrowserPresenter::Select(std::size_t index)
{
	if (index >= EntriesValue.size()) {
		Set_Failure(SaveBrowserFailure::INVALID_SELECTION);
		return(false);
	}

	SelectedIndex = static_cast<int>(index);
	Set_Failure(SaveBrowserFailure::NONE);

	if (ModeValue == SaveBrowserMode::SAVE) {
		FileEntryClass const & entry = EntriesValue[index];
		DescriptionValue = entry.Valid ? Bounded_Description(entry.Descr) : InitialDescriptionValue;
	}
	return(true);
}


SaveBrowserResult SaveBrowserPresenter::Save_Selected(bool allow_overwrite)
{
	FileEntryClass const * entry = Selected_Entry();
	if (entry == nullptr) {
		Set_Failure(SaveBrowserFailure::INVALID_SELECTION);
		return(ResultValue);
	}

	std::string filename;
	if (allow_overwrite && !PendingSaveFilename.empty()) {
		filename = PendingSaveFilename;
	} else if (entry->Valid) {
		filename = entry->Filename;
	} else {
		filename = ServiceValue->Pick_Filename();
	}

	if (filename.empty()) {
		Set_Failure(SaveBrowserFailure::SAVE);
		return(ResultValue);
	}

	if (!allow_overwrite && ServiceValue->Save_Exists(filename)) {
		PendingSaveFilename = filename;
		ConfirmationValue = SaveBrowserConfirmation::OVERWRITE;
		return(ResultValue);
	}

	if (!ServiceValue->Save_File(filename, DescriptionValue)) {
		Set_Failure(SaveBrowserFailure::SAVE);
		return(ResultValue);
	}

	PendingSaveFilename.clear();
	Set_Failure(SaveBrowserFailure::NONE);
	ResultValue = SaveBrowserResult::ACCEPTED;
	return(ResultValue);
}


SaveBrowserResult SaveBrowserPresenter::Delete_Selected(void)
{
	FileEntryClass const * entry = Selected_Entry();
	if (entry == nullptr || !entry->Valid) {
		Set_Failure(SaveBrowserFailure::INVALID_SELECTION);
		return(ResultValue);
	}

	if (!ServiceValue->Delete_File(entry->Filename)) {
		Set_Failure(SaveBrowserFailure::DELETE_FILE);
		return(ResultValue);
	}

	// Do not mutate the model until the delete service reports success.
	std::vector<FileEntryClass> refreshed = ServiceValue->Refresh();
	EntriesValue = std::move(refreshed);
	Set_Failure(SaveBrowserFailure::NONE);

	if (EntriesValue.empty()) {
		SelectedIndex = -1;
		// The retained dialog closes after the last successful delete.
		ResultValue = SaveBrowserResult::ACCEPTED;
	} else {
		// The retained dialog resets the list selection to its first row.
		SelectedIndex = 0;
	}
	return(ResultValue);
}


SaveBrowserResult SaveBrowserPresenter::Accept(void)
{
	if (ResultValue != SaveBrowserResult::PENDING || ConfirmationValue != SaveBrowserConfirmation::NONE) {
		return(ResultValue);
	}

	Set_Failure(SaveBrowserFailure::NONE);

	switch (ModeValue) {
		case SaveBrowserMode::LOAD: {
			FileEntryClass const * entry = Selected_Entry();
			if (entry == nullptr || !entry->Valid) {
				Set_Failure(SaveBrowserFailure::INVALID_SELECTION);
				return(ResultValue);
			}
			if (!ServiceValue->Load_File(entry->Filename)) {
				Set_Failure(SaveBrowserFailure::LOAD);
				return(ResultValue);
			}
			ResultValue = SaveBrowserResult::ACCEPTED;
			return(ResultValue);
		}

		case SaveBrowserMode::SAVE:
			if (!Action_Valid()) {
				Set_Failure(DescriptionValue.empty() ? SaveBrowserFailure::INVALID_DESCRIPTION
					: SaveBrowserFailure::INVALID_SELECTION);
				return(ResultValue);
			}
			return(Save_Selected());

		case SaveBrowserMode::DELETE_MODE:
			if (!Action_Valid()) {
				Set_Failure(SaveBrowserFailure::INVALID_SELECTION);
				return(ResultValue);
			}
			ConfirmationValue = SaveBrowserConfirmation::DELETE_FILE;
			return(ResultValue);
	}

	return(ResultValue);
}


SaveBrowserResult SaveBrowserPresenter::Confirm(void)
{
	if (ConfirmationValue == SaveBrowserConfirmation::NONE) {
		return(ResultValue);
	}

	SaveBrowserConfirmation const confirmation = ConfirmationValue;
	ConfirmationValue = SaveBrowserConfirmation::NONE;
	Set_Failure(SaveBrowserFailure::NONE);

	if (confirmation == SaveBrowserConfirmation::OVERWRITE) {
		return(Save_Selected(true));
	}
	return(Delete_Selected());
}


SaveBrowserResult SaveBrowserPresenter::Reject(void)
{
	if (ConfirmationValue != SaveBrowserConfirmation::NONE) {
		ConfirmationValue = SaveBrowserConfirmation::NONE;
		PendingSaveFilename.clear();
		Set_Failure(SaveBrowserFailure::NONE);
	}
	return(ResultValue);
}


SaveBrowserResult SaveBrowserPresenter::Cancel(void)
{
	if (ResultValue == SaveBrowserResult::PENDING) {
		ConfirmationValue = SaveBrowserConfirmation::NONE;
		PendingSaveFilename.clear();
		ResultValue = SaveBrowserResult::CANCELLED;
	}
	return(ResultValue);
}


void SaveBrowserPresenter::Service(void)
{
	if (ResultValue == SaveBrowserResult::PENDING && ConfirmationValue == SaveBrowserConfirmation::NONE) {
		ServiceValue->Service();
	}
}
