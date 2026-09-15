/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "conquer.h"
#include "loaddlg.h"
#include "savebrowser.h"

#include "campaign.h"
#include "data.h"
#include "gamedirs.h"
#include "globals.h"
#include "init.h"
#include "language/language.h"
#include "msgbox.h"
#include "ownrdraw.h"
#include "saveload.h"
#include "savemgr.h"
#include "session.h"
#include "win.h"
#include "winfix.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <functional>
#include <string>
#include <utility>
#include <vector>


namespace
{


enum class SaveBrowserNativeRequest {
	NONE,
	ACCEPT,
	CANCEL,
};


class LoadOptionsSaveBrowserService final : public SaveBrowserService
{
	public:
		using RefreshFunction = std::function<std::vector<FileEntryClass>(void)>;

		LoadOptionsSaveBrowserService(LoadOptionsClass & options, RefreshFunction refresh) :
			Options(&options),
			RefreshList(std::move(refresh))
		{
		}

		bool Load_File(std::string const & file_name) override
		{
			return(Options->Load_File(file_name.c_str()));
		}

		bool Save_File(std::string const & file_name, std::string const & description) override
		{
			return(Options->Save_File(file_name.c_str(), description.c_str()));
		}

		bool Delete_File(std::string const & file_name) override
		{
			return(Options->Delete_File(file_name.c_str()));
		}

		bool Save_Exists(std::string const & file_name) const override
		{
			std::error_code error;
			return(std::filesystem::exists(Saved_Game_Name(file_name.c_str()), error) && !error);
		}

		std::string Pick_Filename(void) override
		{
			char file_name[256];
			Options->Pick_Filename(file_name);
			return(file_name);
		}

		std::vector<FileEntryClass> Refresh(void) override
		{
			return(RefreshList());
		}

		void Service(void) override
		{
			if (Options->Callback != NULL) {
				Options->Callback();
			}
		}

	private:
		LoadOptionsClass * Options;
		RefreshFunction RefreshList;
};


struct SaveBrowserDialogState {
	SaveBrowserDialogState(LoadOptionsClass & options, SaveBrowserPresenter & presenter,
		LoadOptionsClass::LoadStyleType style, char * description, int save_confirmation) :
		Options(&options),
		Presenter(&presenter),
		Style(style),
		Description(description),
		SaveConfirmation(save_confirmation)
	{
	}

	LoadOptionsClass * Options;
	SaveBrowserPresenter * Presenter;
	LoadOptionsClass::LoadStyleType Style;
	char * Description;
	int SaveConfirmation;
	SaveBrowserNativeRequest Request = SaveBrowserNativeRequest::NONE;
};


SaveBrowserDialogState * Dialog_State(HWND window)
{
	return(reinterpret_cast<SaveBrowserDialogState *>(GetWindowLongPtr(window, DWLP_USER)));
}


SaveBrowserMode Presenter_Mode(LoadOptionsClass::LoadStyleType style)
{
	switch (style) {
		case LoadOptionsClass::LOAD:
			return(SaveBrowserMode::LOAD);
		case LoadOptionsClass::SAVE:
			return(SaveBrowserMode::SAVE);
		case LoadOptionsClass::WWDELETE:
			return(SaveBrowserMode::DELETE_MODE);
		case LoadOptionsClass::NONE:
			break;
	}
	return(SaveBrowserMode::LOAD);
}


std::vector<FileEntryClass> Snapshot_Files(LoadOptionsClass const & options)
{
	std::vector<FileEntryClass> entries;
	for (int index = 0; index < options.Files.Count(); index++) {
		if (options.Files[index] != nullptr) {
			entries.push_back(*options.Files[index]);
		}
	}
	return(entries);
}


int List_Control(LoadOptionsClass::LoadStyleType style)
{
	switch (style) {
		case LoadOptionsClass::LOAD:
			return(IDC_MISSION_LOAD_LIST);
		case LoadOptionsClass::SAVE:
			return(IDC_MISSION_SAVE_LIST);
		case LoadOptionsClass::WWDELETE:
			return(IDC_MISSION_DELETE_LIST);
		case LoadOptionsClass::NONE:
			break;
	}
	return(0);
}


void Render_List(HWND window, SaveBrowserPresenter const & presenter)
{
	OwnerDraw::CellData cell;
	char buffer[128];
	ListBox_ResetContent(window);

	std::vector<FileEntryClass> const & entries = presenter.Entries();
	for (std::size_t index = 0; index < entries.size(); index++) {
		FileEntryClass * entry = const_cast<FileEntryClass *>(&entries[index]);
		int const row = ListBox_AddString(window, entry);

		if (entry->Type != GAME_NORMAL) {
			cell.type = OwnerDraw::CellData::TEXT;
			cell.string.set("*");
			SendMessage(window, OD_SETCELL, MAKEWPARAM(200, row), (LPARAM)&cell);
		}

		if (entry->DateTime != std::chrono::system_clock::time_point::min()) {
			std::time_t const timestamp = std::chrono::system_clock::to_time_t(entry->DateTime);
			std::tm local_time = {};
			if (localtime_s(&local_time, &timestamp) != 0) {
				continue;
			}
			std::strftime(buffer, sizeof(buffer), "%x", &local_time);
			cell.type = OwnerDraw::CellData::TEXT;
			cell.string.set(buffer);
			SendMessage(window, OD_SETCELL, MAKEWPARAM(255, row), (LPARAM)&cell);
			std::strftime(buffer, sizeof(buffer), "%H:%M", &local_time);
			cell.string.set(buffer);
			SendMessage(window, OD_SETCELL, MAKEWPARAM(315, row), (LPARAM)&cell);
		}

		ListBox_SetItemData(window, row, (LPARAM)entry);
	}

	if (presenter.Has_Selection()) {
		int const selection = presenter.Selected_Index();
		ListBox_SetCurSel(window, selection);
		ListBox_SetTopIndex(window, selection);
	}
}


void Set_Save_Description(HWND dialog, SaveBrowserPresenter const & presenter)
{
	HWND edit = GetDlgItem(dialog, IDC_MISSION_SAVE_DESC);
	if (edit != nullptr) {
		SetWindowText(edit, presenter.Description().c_str());
		SetFocus(edit);
		Edit_SetSel(edit, 0, -1);
	}
}


void Focus_Save_Description_End(HWND dialog)
{
	HWND edit = GetDlgItem(dialog, IDC_MISSION_SAVE_DESC);
	if (edit != nullptr) {
		SetFocus(edit);
		Edit_SetSel(edit, -1, -1);
	}
}


void Capture_Save_Description(HWND dialog, SaveBrowserPresenter & presenter)
{
	char buffer[SaveBrowserPresenter::DESCRIPTION_LIMIT + 1];
	buffer[0] = '\0';
	GetWindowText(GetDlgItem(dialog, IDC_MISSION_SAVE_DESC), buffer, sizeof(buffer));
	presenter.Set_Description(buffer);
}


void Show_Presenter_Failure(HWND dialog, SaveBrowserPresenter const & presenter)
{
	switch (presenter.Failure()) {
		case SaveBrowserFailure::INVALID_DESCRIPTION:
			WWMessageBox().Process(TXT_MUSTENTER_DESCRIPTION, TXT_OK, TXT_NONE, TXT_NONE);
			Focus_Save_Description_End(dialog);
			break;
		case SaveBrowserFailure::LOAD:
			WWMessageBox().Process(TXT_ERROR_LOADING_GAME, TXT_OK, TXT_NONE, TXT_NONE);
			break;
		case SaveBrowserFailure::SAVE:
			WWMessageBox().Process(TXT_ERROR_SAVING_GAME, TXT_OK, TXT_NONE, TXT_NONE);
			break;
		case SaveBrowserFailure::DELETE_FILE:
		case SaveBrowserFailure::INVALID_SELECTION:
		case SaveBrowserFailure::NONE:
			break;
	}
}


void Resolve_Confirmation(HWND dialog, SaveBrowserPresenter & presenter)
{
	if (presenter.Pending_Confirmation() == SaveBrowserConfirmation::OVERWRITE) {
		int const native_result = WWMessageBox()._Process(TXT_CONFIRM_SAVE, 1, TXT_YES, TXT_NO, TXT_NONE);
		// WWMessageBox uses zero for its first/YES choice; keep that translation explicit.
		if (native_result == 0) {
			presenter.Confirm();
		} else {
			presenter.Reject();
		}
		return;
	}

	if (presenter.Pending_Confirmation() == SaveBrowserConfirmation::DELETE_FILE) {
		FileEntryClass const * entry = presenter.Selected_Entry();
		char message[256];
		std::snprintf(message, sizeof(message), "%s\n%s", Fetch_String(TXT_DELETE_FILE_QUERY),
			entry == nullptr ? "" : entry->Descr);
		int const native_result = WWMessageBox()._Process(message, 1, TXT_YES, TXT_NO, TXT_NONE);
		// Do not Boolean-simplify this legacy result: zero is semantic confirmation.
		if (native_result == 0) {
			presenter.Confirm();
		} else {
			presenter.Reject();
		}
	}
}


void Apply_Request(HWND dialog, SaveBrowserDialogState & state)
{
	SaveBrowserPresenter & presenter = *state.Presenter;
	SaveBrowserNativeRequest const request = state.Request;
	state.Request = SaveBrowserNativeRequest::NONE;

	if (request == SaveBrowserNativeRequest::CANCEL) {
		presenter.Cancel();
		return;
	}
	if (request != SaveBrowserNativeRequest::ACCEPT) {
		return;
	}

	if (presenter.Mode() == SaveBrowserMode::LOAD) {
		ShowWindow(dialog, SW_HIDE);
		UpdateWindow(MainWindow);
	} else if (presenter.Mode() == SaveBrowserMode::SAVE) {
		Capture_Save_Description(dialog, presenter);
	}

	presenter.Accept();
	Resolve_Confirmation(dialog, presenter);

	if (presenter.Failure() != SaveBrowserFailure::NONE) {
		Show_Presenter_Failure(dialog, presenter);
		if (presenter.Mode() == SaveBrowserMode::LOAD) {
			ShowWindow(dialog, SW_SHOW);
		}
		return;
	}

	if (presenter.Result() == SaveBrowserResult::ACCEPTED && presenter.Mode() == SaveBrowserMode::SAVE) {
		if (state.Description != nullptr) {
			strcpy(state.Description, presenter.Description().c_str());
		}
		int const confirmation = state.SaveConfirmation;
		if (confirmation != TXT_NONE) {
			WWMessageBox().Process(confirmation, TXT_OK, TXT_NONE, TXT_NONE);
		}
	}

	if (presenter.Mode() == SaveBrowserMode::DELETE_MODE &&
		presenter.Result() == SaveBrowserResult::PENDING) {
		Render_List(GetDlgItem(dialog, List_Control(state.Style)), presenter);
		EnableWindow(GetDlgItem(dialog, 1), !presenter.Entries().empty());
	}
}


void On_Command(HWND window, WPARAM wparam, LPARAM lparam)
{
	SaveBrowserDialogState * state = Dialog_State(window);
	if (state == nullptr) {
		return;
	}

	int const id = LOWORD(wparam);
	int const notification = HIWORD(wparam);
	int const list_id = List_Control(state->Style);

	if (id == list_id) {
		HWND list = reinterpret_cast<HWND>(lparam);
		if (list == nullptr) {
			list = GetDlgItem(window, list_id);
		}
		if (notification == LBN_SELCHANGE && list != nullptr) {
			LRESULT const row = ListBox_GetCurSel(list);
			if (row != LB_ERR && state->Presenter->Select(static_cast<std::size_t>(row)) &&
				state->Presenter->Mode() == SaveBrowserMode::SAVE) {
				Set_Save_Description(window, *state->Presenter);
			}
		} else if (notification == LBN_DBLCLK && state->Presenter->Mode() == SaveBrowserMode::LOAD) {
			state->Request = SaveBrowserNativeRequest::ACCEPT;
		}
		return;
	}

	if ((id == IDOK || id == IDCANCEL) && notification == BN_CLICKED) {
		state->Request = id == IDOK ? SaveBrowserNativeRequest::ACCEPT : SaveBrowserNativeRequest::CANCEL;
	}
}


INT_PTR CALLBACK Load_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	INT_PTR result = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);
	if (result == 0) {
		switch (message) {
			case WM_MOVING:
				return(On_WM_MOVING(window, wparam, lparam));
			case WM_COMMAND:
				On_Command(window, wparam, lparam);
				break;
			case OD_SUBCLASSED:
				SendDlgItemMessage(window, IDC_MISSION_LOAD_LIST, OD_ADDCOLUMN, 0xF9, 2);
				SendDlgItemMessage(window, IDC_MISSION_LOAD_LIST, OD_ADDCOLUMN, 0x38, 255);
				SendDlgItemMessage(window, IDC_MISSION_LOAD_LIST, OD_ADDCOLUMN, 0, 315);
				break;
		}
		return(FALSE);
	}
	return(result);
}


INT_PTR CALLBACK Save_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	INT_PTR result = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);
	if (result == 0) {
		switch (message) {
			case WM_MOVING:
				return(On_WM_MOVING(window, wparam, lparam));
			case WM_COMMAND:
				On_Command(window, wparam, lparam);
				break;
			case WM_INITDIALOG:
				SendMessage(GetDlgItem(window, IDC_MISSION_SAVE_DESC), EM_SETLIMITTEXT,
					SaveBrowserPresenter::DESCRIPTION_LIMIT, 0);
				break;
			case OD_SUBCLASSED:
				SendDlgItemMessage(window, IDC_MISSION_SAVE_LIST, OD_ADDCOLUMN, 0xF9, 2);
				SendDlgItemMessage(window, IDC_MISSION_SAVE_LIST, OD_ADDCOLUMN, 0x38, 255);
				SendDlgItemMessage(window, IDC_MISSION_SAVE_LIST, OD_ADDCOLUMN, 0, 315);
				break;
		}
		return(FALSE);
	}
	return(result);
}


INT_PTR CALLBACK Delete_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	INT_PTR result = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);
	if (result == 0) {
		switch (message) {
			case WM_MOVING:
				return(On_WM_MOVING(window, wparam, lparam));
			case WM_COMMAND:
				On_Command(window, wparam, lparam);
				break;
			case OD_SUBCLASSED:
				SendDlgItemMessage(window, IDC_MISSION_DELETE_LIST, OD_ADDCOLUMN, 0xF9, 2);
				SendDlgItemMessage(window, IDC_MISSION_DELETE_LIST, OD_ADDCOLUMN, 0x38, 255);
				SendDlgItemMessage(window, IDC_MISSION_DELETE_LIST, OD_ADDCOLUMN, 0, 315);
				break;
		}
		return(FALSE);
	}
	return(result);
}


} // namespace


bool LoadOptionsClass::Dialog(void)
{
	HWND dialog = nullptr;

	switch (Style) {
		case LOAD:
			dialog = OwnerDraw::Begin_Dialog(IDD_MISSION_LOAD, Load_Dialog_Proc);
			break;
		case SAVE:
			if (Disk_Space_Available() < MinSpaceRequired) {
				WWMessageBox().Process(TXT_DISKFULL, TXT_OK, TXT_NONE, TXT_NONE);
				return(false);
			}
			dialog = OwnerDraw::Begin_Dialog(IDD_MISSION_SAVE, Save_Dialog_Proc);
			break;
		case WWDELETE:
			dialog = OwnerDraw::Begin_Dialog(IDD_MISSION_DELETE, Delete_Dialog_Proc);
			break;
		case NONE:
			break;
	}

	State = STATE_PENDING;
	if (dialog == nullptr) {
		return(false);
	}

	Build_List();
	LoadOptionsSaveBrowserService service(*this, [this]() {
		Build_List();
		return(Snapshot_Files(*this));
	});
	std::vector<FileEntryClass> entries = Snapshot_Files(*this);
	std::string const initial_description = Description == nullptr ? "" : Description;
	SaveBrowserPresenter presenter(Presenter_Mode(Style), std::move(entries), initial_description, service);
	SaveBrowserDialogState state(*this, presenter, Style, Description, Save_Confirmation());
	SetWindowLongPtr(dialog, DWLP_USER, reinterpret_cast<LONG_PTR>(&state));

	HWND list = GetDlgItem(dialog, List_Control(Style));
	if (list != nullptr) {
		Render_List(list, presenter);
		EnableWindow(GetDlgItem(dialog, 1), !presenter.Entries().empty());
	}

	OwnerDraw::Display_Dialog(dialog);
	while (presenter.Result() == SaveBrowserResult::PENDING) {
		if (OwnerDraw::Dialog_Message_Handler()) {
			presenter.Cancel();
			break;
		}

		if (state.Request != SaveBrowserNativeRequest::NONE) {
			Apply_Request(dialog, state);
		} else {
			presenter.Service();
			if (!GameActive) {
				Title_Screen_Restore(0);
			}
		}
	}

	State = presenter.Result() == SaveBrowserResult::ACCEPTED ? STATE_OK : STATE_CLOSE;
	Clear_List();
	OwnerDraw::End_Dialog(dialog);
	return(State == STATE_OK);
}


bool LoadOptionsClass::Load_File(const char * file_name)
{
	HWND dialog = OwnerDraw::Custom_Message_Box(Fetch_String(TXT_LOADING), NULL, NULL);
	if (dialog != 0) {
		OwnerDraw::Display_Dialog(dialog);
	}
	ScenarioActive = false;
	TacticalActive = false;
	bool const loaded = Load_Game(file_name);
	if (dialog != 0) {
		OwnerDraw::End_Dialog(dialog);
	}
	return(loaded);
}


bool LoadOptionsClass::Save_File(const char * file_name, const char * descr)
{
	HWND dialog = OwnerDraw::Custom_Message_Box(Fetch_String(TXT_SAVING_GAME), NULL, NULL);
	if (dialog != 0) {
		OwnerDraw::Display_Dialog(dialog);
	}
	bool const saved = SaveManager.Request_Save_Game(file_name, descr, false,
		SaveManagerClass::NoticeType::Requested);
	if (dialog != 0) {
		OwnerDraw::End_Dialog(dialog);
	}
	return(saved);
}
