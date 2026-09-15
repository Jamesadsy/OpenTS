/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "loaddlg.h"

#include <cstddef>
#include <string>
#include <vector>


/*
 * This is the semantic state owned by the Second Sun save browser. Native controls and
 * platform-specific confirmation plumbing stay in the view adapter.
 */
enum class SaveBrowserMode {
	LOAD,
	SAVE,
	DELETE_MODE,
};

enum class SaveBrowserResult {
	PENDING,
	ACCEPTED,
	CANCELLED,
};

enum class SaveBrowserFailure {
	NONE,
	INVALID_SELECTION,
	INVALID_DESCRIPTION,
	LOAD,
	SAVE,
	DELETE_FILE,
};

enum class SaveBrowserConfirmation {
	NONE,
	OVERWRITE,
	DELETE_FILE,
};


/*
 * The narrow engine-side seam used by the presenter. Implementations may attach the existing
 * game services, or a deterministic recording service in a no-owner-data test.
 */
class SaveBrowserService
{
	public:
		virtual ~SaveBrowserService(void) = default;

		virtual bool Load_File(std::string const & file_name) = 0;
		virtual bool Save_File(std::string const & file_name, std::string const & description) = 0;
		virtual bool Delete_File(std::string const & file_name) = 0;
		virtual bool Save_Exists(std::string const & file_name) const = 0;
		virtual std::string Pick_Filename(void) = 0;
		virtual std::vector<FileEntryClass> Refresh(void) = 0;
		virtual void Service(void) = 0;
};


class SaveBrowserPresenter
{
	public:
		// The Windows edit control accepts 79 characters (plus its terminating null).
		static constexpr std::size_t DESCRIPTION_LIMIT = 79;

		SaveBrowserPresenter(SaveBrowserMode mode, std::vector<FileEntryClass> entries,
			std::string initial_description, SaveBrowserService & service);

		SaveBrowserMode Mode(void) const { return(ModeValue); }
		std::vector<FileEntryClass> const & Entries(void) const { return(EntriesValue); }
		int Selected_Index(void) const { return(SelectedIndex); }
		bool Has_Selection(void) const { return(SelectedIndex >= 0); }
		FileEntryClass const * Selected_Entry(void) const;

		std::string const & Description(void) const { return(DescriptionValue); }
		bool Set_Description(std::string const & description);
		bool Action_Valid(void) const;

		SaveBrowserResult Result(void) const { return(ResultValue); }
		SaveBrowserFailure Failure(void) const { return(FailureValue); }
		SaveBrowserConfirmation Pending_Confirmation(void) const { return(ConfirmationValue); }

		bool Select(std::size_t index);
		SaveBrowserResult Accept(void);
		SaveBrowserResult Confirm(void);
		SaveBrowserResult Reject(void);
		SaveBrowserResult Cancel(void);
		void Service(void);

	private:
		static std::string Bounded_Description(std::string const & description);
		void Set_Failure(SaveBrowserFailure failure);
		SaveBrowserResult Save_Selected(bool allow_overwrite = false);
		SaveBrowserResult Delete_Selected(void);

		SaveBrowserMode ModeValue;
		std::vector<FileEntryClass> EntriesValue;
		int SelectedIndex = -1;
		std::string InitialDescriptionValue;
		std::string DescriptionValue;
		SaveBrowserService * ServiceValue;
		SaveBrowserResult ResultValue = SaveBrowserResult::PENDING;
		SaveBrowserFailure FailureValue = SaveBrowserFailure::NONE;
		SaveBrowserConfirmation ConfirmationValue = SaveBrowserConfirmation::NONE;
		std::string PendingSaveFilename;
};
