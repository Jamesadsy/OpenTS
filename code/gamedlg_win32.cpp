/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "gamedlg.h"

#include "_map.h"
#include "_tooltip.h"
#include "cctooltip.h"
#include "data.h"
#include "dbgprint.h"
#include "audio/audioengine.h"
#include "event.h"
#include "globals.h"
#include "init.h"
#include "language/language.h"
#include "ownrdraw.h"
#include "queue.h"
#include "session.h"
#include "techno.h"

#include "special.hh"


INT_PTR CALLBACK Game_Controls_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam);
void Game_Controls_Dialog_On_COMMAND(HWND window, UINT message, WPARAM wparam, LPARAM lparam);


namespace {

	enum class GameControlsDialogAction {
		CANCEL,
		ACCEPT,
		SOUND,
		KEYBOARD,
		SESSION_ENDED,
	};


	class GameControlsEngineService final : public GameControlsService
	{
		public:
			void Set_Game_Speed(int setting) override
			{
				Options.GameSpeed = setting;
			}

			void Queue_Game_Speed_Event(int setting) override
			{
				OutList.push_back(EventClass(PlayerPtr->HeapID, EventClass::GAMESPEED, setting));
			}

			void Set_Scroll_Rate(int setting) override
			{
				Options.ScrollRate = setting;
			}

			void Set_Detail_Level(int setting) override
			{
				Options.DetailLevel = setting;
			}

			void Reinit_Cell_Drawers(void) override
			{
				Map.Reinit_Cell_Drawers();
			}

			void Set_Sidebar_Cameo_Text(bool enabled) override
			{
				Options.SidebarCameoText = enabled;
			}

			void Toggle_Cameo_Text(bool enabled) override
			{
				Map.Toggle_Cameo_Text(enabled);
			}

			void Set_Action_Lines(bool enabled) override
			{
				Options.ActionLines = enabled;
			}

			void Propagate_Action_Lines(bool enabled) override
			{
				TechnoClass::Set_Action_Lines(enabled);
			}

			void Set_ToolTips(bool enabled) override
			{
				Options.ToolTips = enabled;
			}

			void Activate_ToolTips(bool enabled) override
			{
				if (ToolTips != nullptr) {
					ToolTips->Activate(enabled);
				}
			}

			void Set_Scroll_Method(int method) override
			{
				Options.ScrollMethod = method;
			}

			void Set_Auto_Scroll(bool enabled) override
			{
				Options.AutoScroll = enabled;
			}

			void Set_Difficulty(int difficulty) override
			{
				Options.Difficulty = difficulty;
			}

			void Save_Settings(void) override
			{
				Options.Save_Settings();
			}
	};


	struct GameControlsDialogState {
		GameControlsDialogState(GameControlsContext context, GameControlsState initial, GameControlsService & service) :
			Presenter(context, initial, service)
		{
		}

		GameControlsPresenter Presenter;
		GameControlsDialogAction Action = GameControlsDialogAction::CANCEL;
		int Result = -1;
	};


	GameControlsDialogState * Dialog_State(HWND window)
	{
		return(reinterpret_cast<GameControlsDialogState *>(GetWindowLongPtr(window, DWLP_USER)));
	}


	GameControlsContext Current_Context(void)
	{
		if (!GameActive) {
			return(GameControlsContext::For_Frontend());
		}

		if (Session.Type == GAME_INTERNET) {
			return(GameControlsContext::For_Internet(AudioEngine.Is_Available()));
		}

		bool const local_speed = Session.Type == GAME_NORMAL || Session.Type == GAME_SKIRMISH;
		return(GameControlsContext::For_Session(local_speed, AudioEngine.Is_Available()));
	}


	GameControlsState Current_State(void)
	{
		GameControlsState state;
		state.GameSpeed = Options.GameSpeed;
		state.ScrollRate = Options.ScrollRate;
		state.DetailLevel = Options.DetailLevel;
		state.Difficulty = Options.Difficulty;
		state.SidebarCameoText = Options.SidebarCameoText;
		state.ActionLines = Options.ActionLines;
		state.ToolTips = Options.ToolTips;
		state.Coasting = Options.ScrollMethod == 0;
		state.EdgeScrolling = Options.AutoScroll;
		return(state);
	}


	int Dialog_Resource(void)
	{
		if (!GameActive) {
			return(IDD_OPT_CTRL_GAME_SP);
		}
		return(Session.Type == GAME_INTERNET ? IDD_OPT_CTRL_GAME_WOL : IDD_OPT_CTRL_GAME_MP);
	}


	void Initialize_Controls(HWND window)
	{
		HWND handle = GetDlgItem(window, IDC_GAME_SPEED_SLIDER);
		if (handle) {
			SendMessage(handle, OD_TRACKNUMBERS, 0, 0);
			Slider_SetRange(handle, 0, OptionsClass::MAX_SPEED_SETTING - 1);
			Slider_SetPos(handle, GameControlsPresenter::Game_Speed_Position(Options.GameSpeed));
		}

		handle = GetDlgItem(window, IDC_SCROLL_SPEED_SLIDER);
		if (handle) {
			SendMessage(handle, OD_TRACKNUMBERS, 0, 0);
			Slider_SetRange(handle, 0, OptionsClass::MAX_SCROLL_SETTING - 1);
			Slider_SetPos(handle, GameControlsPresenter::Scroll_Rate_Position(Options.ScrollRate));
		}

		handle = GetDlgItem(window, IDC_DETAIL_LEVEL_SLIDER);
		if (handle) {
			SendMessage(handle, OD_TRACKNUMBERS, 0, 0);
			Slider_SetRange(handle, 0, OptionsClass::MAX_DETAIL_SETTING - 1);
			Slider_SetPos(handle, Options.DetailLevel);
		}

		handle = GetDlgItem(window, IDC_SIDEBAR_TEXT);
		if (handle) {
			Button_SetCheck(handle, Options.SidebarCameoText != false);
		}

		handle = GetDlgItem(window, IDC_TARGET_LINES);
		if (handle) {
			Button_SetCheck(handle, Options.ActionLines != false);
		}

		handle = GetDlgItem(window, IDC_TOOLTIPS);
		if (handle) {
			Button_SetCheck(handle, Options.ToolTips != false);
		}

		handle = GetDlgItem(window, IDC_SCROLL_COASTING);
		if (handle) {
			Button_SetCheck(handle, Options.ScrollMethod == 0);
		}

		handle = GetDlgItem(window, IDC_EDGE_SCROLL);
		if (handle) {
			Button_SetCheck(handle, Options.AutoScroll != false);
		}

		if (GameActive) {
			handle = GetDlgItem(window, IDC_OPT_SOUND_BTN);
			if (handle) {
				EnableWindow(handle, AudioEngine.Is_Available());
			}
		} else {
			handle = GetDlgItem(window, IDC_DIFFICULTY_SLIDER);
			if (handle) {
				SendMessage(handle, OD_TRACKNUMBERS, 0, 0);
				Slider_SetRange(handle, 0, OptionsClass::MAX_DIFFICULTY_SETTING - 1);
				Slider_SetPos(handle, Options.Difficulty);
			}
		}
	}


	void Capture_Controls(HWND window, GameControlsPresenter & presenter)
	{
		HWND handle = GetDlgItem(window, IDC_GAME_SPEED_SLIDER);
		if (handle && presenter.Has_Speed()) {
			presenter.Set_Game_Speed_Position(Slider_GetPos(handle));
		}

		handle = GetDlgItem(window, IDC_SCROLL_SPEED_SLIDER);
		if (handle) {
			presenter.Set_Scroll_Rate_Position(Slider_GetPos(handle));
		}

		handle = GetDlgItem(window, IDC_DETAIL_LEVEL_SLIDER);
		if (handle) {
			presenter.Set_Detail_Position(Slider_GetPos(handle));
		}

		handle = GetDlgItem(window, IDC_SIDEBAR_TEXT);
		if (handle) {
			presenter.Set_Sidebar_Cameo_Text(Button_GetCheck(handle) == TRUE);
		}

		handle = GetDlgItem(window, IDC_TARGET_LINES);
		if (handle) {
			presenter.Set_Action_Lines(Button_GetCheck(handle) == TRUE);
		}

		handle = GetDlgItem(window, IDC_TOOLTIPS);
		if (handle) {
			presenter.Set_ToolTips(Button_GetCheck(handle) == TRUE);
		}

		handle = GetDlgItem(window, IDC_SCROLL_COASTING);
		if (handle) {
			presenter.Set_Coasting(Button_GetCheck(handle) == TRUE);
		}

		handle = GetDlgItem(window, IDC_EDGE_SCROLL);
		if (handle) {
			presenter.Set_Edge_Scrolling(Button_GetCheck(handle) == TRUE);
		}

		handle = GetDlgItem(window, IDC_DIFFICULTY_SLIDER);
		if (handle && presenter.Has_Difficulty()) {
			presenter.Set_Difficulty_Position(Slider_GetPos(handle));
		}
	}


	GameControlsResult Apply_Action(GameControlsDialogState & state)
	{
		switch (state.Action) {
			case GameControlsDialogAction::ACCEPT:
				return(state.Presenter.Accept());

			case GameControlsDialogAction::SOUND:
				return(state.Presenter.Request_Sound());

			case GameControlsDialogAction::KEYBOARD:
				return(state.Presenter.Request_Keyboard());

			case GameControlsDialogAction::CANCEL:
			case GameControlsDialogAction::SESSION_ENDED:
				return(state.Action == GameControlsDialogAction::SESSION_ENDED
					? state.Presenter.Session_Ended()
					: state.Presenter.Cancel());
		}

		return(GameControlsResult::CANCELLED);
	}

} // namespace


void GameControlsClass::Dialog(void)
{
	GameControlsEngineService service;
	GameControlsDialogState state(Current_Context(), Current_State(), service);

	DebugString("GameControls: GameSpeed = %d, ScrollRate = %d, Detail = %d\n", Options.GameSpeed, Options.ScrollRate, Options.DetailLevel);

	HWND dialog = OwnerDraw::Begin_Dialog(Dialog_Resource(), Game_Controls_Dialog_Proc);
	if (dialog) {
		SetWindowLongPtr(dialog, DWLP_USER, reinterpret_cast<LONG_PTR>(&state));
		OwnerDraw::Display_Dialog(dialog);

		while (state.Result == -1) {
			if (OwnerDraw::Dialog_Message_Handler()) {
				state.Action = GameControlsDialogAction::SESSION_ENDED;
				state.Result = IDCANCEL;
			}
			if (!GameActive) {
				Title_Screen_Restore();
			}
		}

		GameControlsResult result = GameControlsResult::CANCELLED;
		if (state.Result == IDOK) {
			Capture_Controls(dialog, state.Presenter);
			result = Apply_Action(state);
		}

		OwnerDraw::End_Dialog(dialog);

		if (result == GameControlsResult::SOUND) {
			SpecialDialog = SDLG_SOUND;
		} else if (result == GameControlsResult::KEYBOARD) {
			SpecialDialog = SDLG_KEYBOARD;
		}
	}

	DebugString("GameControls: GameSpeed = %d, ScrollRate = %d, Detail = %d\n", Options.GameSpeed, Options.ScrollRate, Options.DetailLevel);
}


INT_PTR CALLBACK Game_Controls_Dialog_Proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	HWND handle;
	int index;

	INT_PTR rc = OwnerDraw::Default_Dialog_Proc(window, message, wparam, lparam);
	if (rc == 0) {
		switch (message) {
			case WM_INITDIALOG:
				Initialize_Controls(window);
				break;

			case WM_COMMAND:
				Game_Controls_Dialog_On_COMMAND(window, LOWORD(wparam), 0, HIWORD(wparam));
				break;

			case WM_HSCROLL:
				if (LOWORD(wparam) == SB_THUMBTRACK) {
					index = HIWORD(wparam);
					int name = TXT_NONE;

					handle = nullptr;
					GameControlsDialogState * state = Dialog_State(window);
					if (reinterpret_cast<HWND>(lparam) == GetDlgItem(window, IDC_GAME_SPEED_SLIDER)) {
						name = GameSpeedNames[index];
						handle = GetDlgItem(window, IDC_GAME_SPEED_LABEL);
						if (state != nullptr) {
							state->Presenter.Set_Game_Speed_Position(index);
						}
					} else if (reinterpret_cast<HWND>(lparam) == GetDlgItem(window, IDC_SCROLL_SPEED_SLIDER)) {
						name = GameScrollSpeedNames[index];
						handle = GetDlgItem(window, IDC_SCROLL_SPEED_LABEL);
						if (state != nullptr) {
							state->Presenter.Set_Scroll_Rate_Position(index);
						}
					} else if (reinterpret_cast<HWND>(lparam) == GetDlgItem(window, IDC_DETAIL_LEVEL_SLIDER)) {
						name = GameDetailLevelNames[index];
						handle = GetDlgItem(window, IDC_DETAIL_LEVEL_LABEL);
						if (state != nullptr) {
							state->Presenter.Set_Detail_Position(index);
						}
					} else if (!GameActive && reinterpret_cast<HWND>(lparam) == GetDlgItem(window, IDC_DIFFICULTY_SLIDER)) {
						name = GameDifficultyNames[index];
						handle = GetDlgItem(window, IDC_DIFFICULTY_LABEL);
						if (state != nullptr) {
							state->Presenter.Set_Difficulty_Position(index);
						}
					}
					if (handle) {
						SetWindowText(handle, Fetch_String(name));
					}
				}
				break;
		}
		rc = 0;
	}
	return(rc);
}


void Game_Controls_Dialog_On_COMMAND(HWND window, UINT message, WPARAM, LPARAM lparam)
{
	GameControlsDialogState * state = Dialog_State(window);
	if (state == nullptr) {
		return;
	}

	switch (static_cast<int>(message)) {
		case IDC_OPT_KEYBOARD_BTN:
			if (lparam == 0 && GameActive && state->Presenter.Can_Request_Keyboard()) {
				state->Action = GameControlsDialogAction::KEYBOARD;
				state->Result = IDOK;
			}
			break;

		case IDC_OPT_SOUND_BTN:
			if (lparam == 0 && GameActive && state->Presenter.Can_Request_Sound()) {
				state->Action = GameControlsDialogAction::SOUND;
				state->Result = IDOK;
			}
			break;

		case IDOK:
			if (lparam == 0) {
				state->Action = GameControlsDialogAction::ACCEPT;
				state->Result = IDOK;
			}
			break;

		case IDCANCEL:
			state->Action = GameControlsDialogAction::CANCEL;
			state->Result = IDCANCEL;
			break;
	}
}
