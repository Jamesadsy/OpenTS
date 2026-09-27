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

#pragma once

#include "stimer.h"

#include "side.hh"
#include "theater.hh"
#include "theme.hh"

#include <cstdint>
#include <memory>

template<class T> class TRect;
typedef TRect<int> Rect;
class Surface;
class ObjectClass;

bool Parse_Command_Line(int argc, char * argv[]);

int Init_Game(int argc, char * argv[]);
bool Select_Game(bool fade);

enum class SelectGamePhase : std::uint8_t {
	Idle,
	Reset,
	MainMenu,
	ChildModal,
	ThemeDrain,
	IntroMovie,
	ScenarioPrepare,
	ScenarioChild,
	ConnectionSetup,
	InitialPresentation,
	NativeChild,
	ReadyForGameplay,
	Exit,
	Fault,
};

enum class SelectGameStepResult : std::uint8_t {
	Waiting,
	NativeChild,
	ReadyForGameplay,
	Exited,
	Fault,
};

enum class SelectGameCampaignOutcome : std::uint8_t {
	Pending,
	Accepted,
	Cancelled,
	PresentationFailure,
};

// Native owner for the frontend selection parent. The browser host retains this object
// between callbacks; its typed screen child and every value crossing a Step live here.
class SelectGameSession
{
	public:
		SelectGameSession(void);
		~SelectGameSession(void);

		SelectGameSession(SelectGameSession const &) = delete;
		SelectGameSession & operator=(SelectGameSession const &) = delete;

		void Begin(bool fade, bool native_blocking_adapter = false);
		void Reset(void);
		SelectGameStepResult Step(void);
		bool Complete_Native_Child(
			bool process,
			int selection,
			bool game_loaded,
			bool selection_from_menu = false);

		bool GameLoaded = false;
		int Selection = 0;
		bool Process = true;
		bool Fade = false;
		std::uint32_t WaitDeadlineMs = 0;
		int Campaign = -1;
		int Difficulty = 0;
		SelectGameCampaignOutcome CampaignOutcome = SelectGameCampaignOutcome::Pending;
		SelectGamePhase Phase = SelectGamePhase::Idle;

	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
		friend bool Select_Game(bool fade);
};

int Main_Menu(unsigned int timeout);

bool Allocate_Surfaces(const Rect & hidden_rect, const Rect & composite_rect, const Rect & tile_rect, const Rect & sidebar_rect, bool hidden_first = false);

bool Init_Hotkeys(void);
void Execute_Command(char const * name);
void Reset_Selection_Filters(void);

#define ATTRACT_MODE_TIMEOUT	TIMER_MINUTE

void Title_Screen_Restore(bool force=false);

// Spells a cheat word out one character at a time and answers when one is completed. The
// main menu screen owns this; it is not a key binding and never reaches the command list.
bool Cheat_Key_Process(char chr);

void Init_Campaigns(void);

void Prepare_Theater_Roster(void);
void Prepare_Side_Roster(void);

void Delete_All_Objects(void);

void Init_Theater(TheaterType theater);
bool Prep_For_Side(SideType side);
bool Prep_Speech_For_Side(SideType side);
SideType Prep_For_Side_Or_First(SideType side);
SideType Prep_Speech_For_Side_Or_First(SideType side);

void Anim_Init(void);

void Load_Title_Page(const char * name, bool visible);

ThemeType Fetch_Main_Menu_Theme(void);
ThemeType Fetch_Map_Select_Theme(void);

void Draw_Version_Text(Surface * surface);

ObjectClass * Best_Selected_Object(void);
int New_Main_Menu(void);
