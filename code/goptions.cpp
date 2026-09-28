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

/* $Header: /counterstrike/GOPTIONS.CPP 6     3/15/97 7:18p Steve_tall $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : OPTIONS.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : June 8, 1994                                                 *
 *                                                                                             *
 *                  Last Update : July 27, 1995 [JLB]                                          *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   OptionsClass::Process -- Handles all the options graphic interface.                       *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "goptions.h"

#include "_keyboar.h"
#include "_map.h"
#include "data.h"
#include "dbgprint.h"
#include "gamedlg.h"
#include "language/language.h"
#include "loaddlg.h"
#include "queue.h"
#include "restate.h"
#include "savemgr.h"
#include "scenario.h"
#include "stats.h"
#include "ui/uiabort.h"
#include "ui/uifrontendcontinuation.h"
#include "ui/uigameoptions.h"

#include "special.hh"

int Network_Quality_Text_ID(NetTiming::ConnectionQuality quality)
{
	switch (quality) {
		case NetTiming::ConnectionQuality::Fast: return(TXT_BEST_CONNECTION);
		case NetTiming::ConnectionQuality::Normal: return(TXT_GOOD_CONNECTION);
		case NetTiming::ConnectionQuality::Poor: return(TXT_POOR_CONNECTION);
		case NetTiming::ConnectionQuality::Bad: return(TXT_WORST_CONNECTION);
	}
	return(TXT_WORST_CONNECTION);
}

// What the driver does on the way out. The briefing is restated after the screen has gone,
// which is where the dialog driver restated it.
static void Game_Options_Finish(UIGameOptionsPresenterClass const & screen)
{
	Keyboard->Clear();

	if (screen.Choice == UIGameOptionsPresenterClass::CHOICE_BRIEFING) {
		Restate_Mission(Scen);
	}

	IgnoreInput = Scen->IsInputLocked;

	if (screen.Choice == UIGameOptionsPresenterClass::CHOICE_LOADED) {
		if (MouseCursor->Is_Hidden() == false && Scen->IsInputLocked == 1) {
			Hide_Mouse();
		} else if (MouseCursor->Is_Hidden() == true && Scen->IsInputLocked == 0) {
			Show_Mouse();
		}
	}

	Map.Flag_To_Redraw(GS_REDRAW_ALL);
}


/// <summary>
/// Displays the in game options dialog.
/// This routine is used by the special dialog handler when the player calls up the options
/// screen. Which layout appears depends on the kind of game in progress. Game input stays
/// locked out for as long as the screen is up, and if the player asked for the mission
/// briefing it is restated on the way out.
/// </summary>
void Game_Options_Dialog(void)
{
	std::unique_ptr<UIFrontendContinuation> continuation = Game_Options_Dialog_Continuation(true);
	UI_Run_Frontend_Continuation(*continuation);
}


namespace
{

class GameOptionsDialogContinuation final : public UIFrontendContinuation
{
	public:
		explicit GameOptionsDialogContinuation(bool native_blocking_adapter)
		{
			m_Presenter.Refresh();
			IgnoreInput = true;
			Keyboard->Clear();
			m_Screen = UI_Game_Options_Screen_Continuation(m_Presenter, native_blocking_adapter);
		}

		~GameOptionsDialogContinuation(void) override
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
			} else {
				m_LastStep = m_Screen->Step();
				if (m_LastStep == OpenTS_UIModal::UIModalStepResult::Complete
					|| m_LastStep == OpenTS_UIModal::UIModalStepResult::SessionEnded
					|| m_LastStep == OpenTS_UIModal::UIModalStepResult::Fault) {
					m_Result = m_Screen->Result();
					if (!m_Screen->Finish()) m_Result.Outcome = UIResult::OUTCOME_SESSION_ENDED;
					m_Screen.reset();
					m_Completed = true;
				}
			}
			if (m_Completed && !m_PostApplied) {
				Game_Options_Finish(m_Presenter);
				m_PostApplied = true;
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
			if (m_Completed && !m_PostApplied) {
				Game_Options_Finish(m_Presenter);
				m_PostApplied = true;
			}
			m_Finished = true;
			return(success);
		}

	private:
		UIGameOptionsPresenterClass m_Presenter;
		std::unique_ptr<UIFrontendContinuation> m_Screen;
		UIResult m_Result;
		OpenTS_UIModal::UIModalStepResult m_LastStep = OpenTS_UIModal::UIModalStepResult::Active;
		bool m_Completed = false;
		bool m_PostApplied = false;
		bool m_Finished = false;
};


class AbortDialogContinuation final : public UIFrontendContinuation
{
	public:
		explicit AbortDialogContinuation(bool native_blocking_adapter)
		{
			m_Presenter.Refresh();
			m_Screen = UI_Abort_Screen_Continuation(m_Presenter, native_blocking_adapter);
		}

		~AbortDialogContinuation(void) override
		{
			if (!m_Finished) Finish();
		}

		OpenTS_UIModal::UIModalStepResult Step(void) noexcept override
		{
			if (m_Completed || m_Finished) return(m_LastStep);
			if (m_Screen == nullptr) {
				m_Result.Outcome = UIResult::OUTCOME_FAILED_TO_OPEN;
				m_Result.Value = (int)UIAbortDialogOutcome::PRESENTATION_FAILURE;
				m_Completed = true;
				m_LastStep = OpenTS_UIModal::UIModalStepResult::Complete;
				return(m_LastStep);
			}
			m_LastStep = m_Screen->Step();
			if (m_LastStep == OpenTS_UIModal::UIModalStepResult::Complete
				|| m_LastStep == OpenTS_UIModal::UIModalStepResult::SessionEnded
				|| m_LastStep == OpenTS_UIModal::UIModalStepResult::Fault) {
				UIResult const presentation = m_Screen->Result();
				UIAbortDialogOutcome const outcome = UI_Abort_Dialog_Result(presentation, m_Presenter.Choice);
				m_Result = presentation;
				m_Result.Value = (int)outcome;
				if (outcome == UIAbortDialogOutcome::PRESENTATION_FAILURE) {
					m_Result.Outcome = UIResult::OUTCOME_FAILED_TO_OPEN;
				} else if (outcome == UIAbortDialogOutcome::CANCEL) {
					m_Result.Outcome = UIResult::OUTCOME_CANCELLED;
				} else {
					m_Result.Outcome = UIResult::OUTCOME_ACCEPTED;
				}
				if (!m_Screen->Finish()) m_Result.Outcome = UIResult::OUTCOME_SESSION_ENDED;
				m_Screen.reset();
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
				if (!m_Completed) {
					m_Result = m_Screen->Result();
					m_Result.Value = (int)UI_Abort_Dialog_Result(m_Result, m_Presenter.Choice);
				}
				success = m_Screen->Finish();
				m_Screen.reset();
			}
			m_Finished = true;
			return(success);
		}

	private:
		UIAbortPresenterClass m_Presenter;
		std::unique_ptr<UIFrontendContinuation> m_Screen;
		UIResult m_Result;
		OpenTS_UIModal::UIModalStepResult m_LastStep = OpenTS_UIModal::UIModalStepResult::Active;
		bool m_Completed = false;
		bool m_Finished = false;
};

} // namespace


std::unique_ptr<UIFrontendContinuation> Game_Options_Dialog_Continuation(bool native_blocking_adapter)
{
	return(std::make_unique<GameOptionsDialogContinuation>(native_blocking_adapter));
}


/// <summary>
/// Displays the abort mission dialog and waits for an answer.
/// This routine is used by the special dialog handler when the player asks to abandon or
/// surrender the mission. It does not return until the player has settled on one of the
/// choices offered.
/// </summary>
/// <returns>Returns an explicit quit, restart/surrender, cancel, or presentation-failure
/// outcome.</returns>
UIAbortDialogOutcome Abort_Dialog(void)
{
	std::unique_ptr<UIFrontendContinuation> continuation = Abort_Dialog_Continuation(true);
	UIResult const result = UI_Run_Frontend_Continuation(*continuation);
	return((UIAbortDialogOutcome)result.Value);
}


std::unique_ptr<UIFrontendContinuation> Abort_Dialog_Continuation(bool native_blocking_adapter)
{
	return(std::make_unique<AbortDialogContinuation>(native_blocking_adapter));
}
