/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

enum class GameControlsMode {
	FRONTEND,
	SESSION,
	INTERNET,
};

enum class GameControlsSpeedPolicy {
	DIRECT,
	NETWORK_EVENT,
	DISABLED,
};

enum class GameControlsResult {
	PENDING,
	ACCEPTED,
	CANCELLED,
	SOUND,
	KEYBOARD,
};

struct GameControlsContext {
	GameControlsMode Mode = GameControlsMode::FRONTEND;
	GameControlsSpeedPolicy SpeedPolicy = GameControlsSpeedPolicy::DIRECT;
	bool HasSpeed = true;
	bool HasDifficulty = false;
	bool HasSound = false;
	bool HasKeyboard = false;
	bool SoundAvailable = false;

	static GameControlsContext For_Frontend(void);
	static GameControlsContext For_Session(bool local_speed, bool sound_available);
	static GameControlsContext For_Internet(bool sound_available);
};


struct GameControlsState {
	int GameSpeed = 0;
	int ScrollRate = 0;
	int DetailLevel = 0;
	int Difficulty = 0;
	bool SidebarCameoText = false;
	bool ActionLines = false;
	bool ToolTips = false;
	bool Coasting = false;
	bool EdgeScrolling = false;
};


class GameControlsService
{
	public:
		virtual ~GameControlsService(void) = default;

		virtual void Set_Game_Speed(int setting) = 0;
		virtual void Queue_Game_Speed_Event(int setting) = 0;
		virtual void Set_Scroll_Rate(int setting) = 0;
		virtual void Set_Detail_Level(int setting) = 0;
		virtual void Reinit_Cell_Drawers(void) = 0;
		virtual void Set_Sidebar_Cameo_Text(bool enabled) = 0;
		virtual void Toggle_Cameo_Text(bool enabled) = 0;
		virtual void Set_Action_Lines(bool enabled) = 0;
		virtual void Propagate_Action_Lines(bool enabled) = 0;
		virtual void Set_ToolTips(bool enabled) = 0;
		virtual void Activate_ToolTips(bool enabled) = 0;
		virtual void Set_Scroll_Method(int method) = 0;
		virtual void Set_Auto_Scroll(bool enabled) = 0;
		virtual void Set_Difficulty(int difficulty) = 0;
		virtual void Save_Settings(void) = 0;
};


class GameControlsPresenter
{
	public:
		static constexpr int MAX_SPEED_SETTING = 7;
		static constexpr int MAX_SCROLL_SETTING = 7;
		static constexpr int MAX_DETAIL_SETTING = 3;
		static constexpr int MAX_DIFFICULTY_SETTING = 3;

		GameControlsPresenter(GameControlsContext context, GameControlsState initial, GameControlsService & service);

		static int Game_Speed_From_Position(int position);
		static int Game_Speed_Position(int setting);
		static int Scroll_Rate_From_Position(int position);
		static int Scroll_Rate_Position(int setting);

		GameControlsContext const & Context(void) const { return(ContextValue); }
		GameControlsState const & State(void) const { return(StateValue); }
		GameControlsResult Result(void) const { return(ResultValue); }

		bool Has_Speed(void) const { return(ContextValue.HasSpeed); }
		bool Has_Difficulty(void) const { return(ContextValue.HasDifficulty); }
		bool Has_Sound(void) const { return(ContextValue.HasSound); }
		bool Has_Keyboard(void) const { return(ContextValue.HasKeyboard); }
		bool Can_Request_Sound(void) const;
		bool Can_Request_Keyboard(void) const;

		int Game_Speed_Position(void) const;
		int Scroll_Rate_Position(void) const;

		void Set_Game_Speed_Position(int position);
		void Set_Scroll_Rate_Position(int position);
		void Set_Detail_Position(int position);
		void Set_Difficulty_Position(int position);
		void Set_Sidebar_Cameo_Text(bool enabled);
		void Set_Action_Lines(bool enabled);
		void Set_ToolTips(bool enabled);
		void Set_Coasting(bool enabled);
		void Set_Edge_Scrolling(bool enabled);

		GameControlsResult Accept(void);
		GameControlsResult Cancel(void);
		GameControlsResult Session_Ended(void);
		GameControlsResult Request_Sound(void);
		GameControlsResult Request_Keyboard(void);

	private:
		void Apply(void);
		GameControlsResult Commit(GameControlsResult result);

		GameControlsContext ContextValue;
		GameControlsState InitialState;
		GameControlsState StateValue;
		GameControlsService * Service;
		GameControlsResult ResultValue = GameControlsResult::PENDING;
};
