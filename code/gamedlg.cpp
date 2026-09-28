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

/* $Header: /CounterStrike/GAMEDLG.CPP 1     3/03/97 10:24a Joe_bostic $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : GAMEDLG.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Maria del Mar McCready Legg, Joe L. Bostic                   *
 *                                                                                             *
 *                   Start Date : Jan 8, 1995                                                  *
 *                                                                                             *
 *                  Last Update : Jan 18, 1995   [MML]                                         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   OptionsClass::Process -- Handles all the options graphic interface.                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "gamedlg.h"

#include "ui/uifrontendcontinuation.h"
#include "ui/uigamecontrols.h"

#include "_map.h"
#include "_tooltip.h"
#include "cctooltip.h"
#include "data.h"
#include "dbgprint.h"
#include "audio/audioengine.h"
#include "globals.h"
#include "init.h"
#include "language/language.h"
#include "queue.h"
#include "session.h"
#include "techno.h"
#include "ui/uigamecontrols.h"

#include "special.hh"

#include <string>
#include <vector>

int GameSpeedNames[OptionsClass::MAX_SPEED_SETTING] = {
	TXT_SLOWEST,
	TXT_SLOWER,
	TXT_SLOW,
	TXT_MEDIUM,
	TXT_FAST,
	TXT_FASTER,
	TXT_FASTEST
};

int GameScrollSpeedNames[OptionsClass::MAX_SCROLL_SETTING] = {
	TXT_SLOWEST,
	TXT_SLOWER,
	TXT_SLOW,
	TXT_MEDIUM,
	TXT_MEDIUM,
	TXT_FAST,
	TXT_FASTER,
	TXT_FASTEST
};

int GameDetailLevelNames[OptionsClass::MAX_DETAIL_SETTING] = {
	TXT_LOW,
	TXT_MEDIUM,
	TXT_HIGH
};

int GameDifficultyNames[OptionsClass::MAX_DIFFICULTY_SETTING] = {
	TXT_EASY,
	TXT_NORMAL,
	TXT_HARD
};


/***********************************************************************************************
 * OptionsClass::Process -- Handles all the options graphic interface.                         *
 *                                                                                             *
 *    This routine is the main control for the visual representation of the options            *
 *    screen. It handles the visual overlay and the player input.                              *
 *                                                                                             *
 * INPUT:   none                                                                               *
 * OUTPUT:  none                                                                               *
 * WARNINGS:   none                                                                            *
 * HISTORY:                                                                                    *
 *   12/31/1994 MML : Created.                                                                 *
 *=============================================================================================*/
void GameControlsClass::Dialog(void)
{
	DebugString("GameControls: GameSpeed = %d, ScrollRate = %d, Detail = %d\n", Options.GameSpeed, Options.ScrollRate, Options.DetailLevel);
	std::unique_ptr<UIFrontendContinuation> continuation = GameControls_Dialog_Continuation(true);
	UI_Run_Frontend_Continuation(*continuation);

	DebugString("GameControls: GameSpeed = %d, ScrollRate = %d, Detail = %d\n", Options.GameSpeed, Options.ScrollRate, Options.DetailLevel);
}


namespace
{

class GameControlsDialogContinuation final : public UIFrontendContinuation
{
	public:
		explicit GameControlsDialogContinuation(bool native_blocking_adapter)
		{
			m_Presenter.Refresh();
			m_Screen = UI_Game_Controls_Screen_Continuation(m_Presenter, native_blocking_adapter);
		}

		~GameControlsDialogContinuation(void) override
		{
			if (!m_Finished) Finish();
		}

		OpenTS_UIModal::UIModalStepResult Step(void) noexcept override
		{
			if (m_Completed || m_Finished) return(m_LastStep);
			if (m_Screen == nullptr) {
				m_Result.Outcome = UIResult::OUTCOME_FAILED_TO_OPEN;
				m_Completed = true;
				m_LastStep = OpenTS_UIModal::UIModalStepResult::Complete;
				return(m_LastStep);
			}
			m_LastStep = m_Screen->Step();
			if (m_LastStep == OpenTS_UIModal::UIModalStepResult::Complete
				|| m_LastStep == OpenTS_UIModal::UIModalStepResult::SessionEnded) {
				m_Result = m_Screen->Result();
				if (!m_Applied && m_Result.Outcome != UIResult::OUTCOME_FAILED_TO_OPEN
					&& m_Presenter.Commits()) {
					m_Presenter.Apply();
					Options.Save_Settings();
					m_Applied = true;
				}
				m_Completed = true;
			}
			return(m_LastStep);
		}

		UIResult const & Result(void) const noexcept override { return(m_Result); }

		bool Finish(void) noexcept override
		{
			if (m_Finished) return(true);
			bool success = true;
			if (m_Screen != nullptr) {
				if (!m_Completed) m_Result = m_Screen->Result();
				success = m_Screen->Finish();
				m_Screen.reset();
			}
			m_Finished = true;
			return(success);
		}

	private:
		UIGameControlsPresenterClass m_Presenter;
		std::unique_ptr<UIFrontendContinuation> m_Screen;
		UIResult m_Result;
		OpenTS_UIModal::UIModalStepResult m_LastStep = OpenTS_UIModal::UIModalStepResult::Active;
		bool m_Applied = false;
		bool m_Completed = false;
		bool m_Finished = false;
};

} // namespace


std::unique_ptr<UIFrontendContinuation> GameControls_Dialog_Continuation(bool native_blocking_adapter)
{
	return(std::make_unique<GameControlsDialogContinuation>(native_blocking_adapter));
}
