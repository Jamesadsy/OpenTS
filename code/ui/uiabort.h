/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// The abort and surrender screen's behavior, with no toolkit in it. One screen serves both:
// the middle choice is a restart in a solo mission and a surrender in a session, which is
// what IDD_MISSION_ABORT's own procedure decided at WM_INITDIALOG.
//
// docs/UI_DESIGN.md, "Screens", owns the contract.

#pragma once

#include "uiscreen.h"

#include <string>


inline constexpr char const * UI_ABORT_QUIT = "quit";
inline constexpr char const * UI_ABORT_RESTART = "restart";
inline constexpr char const * UI_ABORT_CANCEL = "cancel";


class UIAbortPresenterClass : public UIPresenterClass
{
	public:
		enum ChoiceType {
			CHOICE_NONE,
			CHOICE_QUIT,
			CHOICE_RESTART,
			CHOICE_CANCEL,
		};

		virtual void Execute(UIIntent const & intent) override;
		virtual void Refresh(void) override;

		/*
		**	The view-model. Plain values, and the only thing a view reads.
		*/

		// What the middle button should be relabelled to, or empty to leave the caption the
		// view already carries. Only the surrender overrides it; the restart is the caption
		// the template holds, and the template is the localized resource.
		std::string RestartCaption;

		// Is the middle choice offered at all? A player whose fate is already settled cannot
		// surrender, which is what the dialog disabled the button for.
		bool CanRestart = true;

		ChoiceType Choice = CHOICE_NONE;
};


enum class UIAbortDialogOutcome {
	QUIT,
	RESTART_OR_SURRENDER,
	CANCEL,
	PRESENTATION_FAILURE,
};


// Resolve both the presenter's choice and the view result. A missing or failed view is an
// explicit non-success, never the old default-zero/no-action answer.
inline UIAbortDialogOutcome UI_Abort_Dialog_Result(
	UIResult const & presentation, UIAbortPresenterClass::ChoiceType choice)
{
	if (presentation.Outcome == UIResult::OUTCOME_FAILED_TO_OPEN ||
		presentation.Outcome == UIResult::OUTCOME_SESSION_ENDED) {
		return(UIAbortDialogOutcome::PRESENTATION_FAILURE);
	}

	switch (choice) {
		case UIAbortPresenterClass::CHOICE_QUIT:
			return(presentation.Outcome == UIResult::OUTCOME_ACCEPTED
				? UIAbortDialogOutcome::QUIT
				: UIAbortDialogOutcome::PRESENTATION_FAILURE);

		case UIAbortPresenterClass::CHOICE_RESTART:
			return(presentation.Outcome == UIResult::OUTCOME_ACCEPTED
				? UIAbortDialogOutcome::RESTART_OR_SURRENDER
				: UIAbortDialogOutcome::PRESENTATION_FAILURE);

		case UIAbortPresenterClass::CHOICE_CANCEL:
			return(presentation.Outcome == UIResult::OUTCOME_CANCELLED
				? UIAbortDialogOutcome::CANCEL
				: UIAbortDialogOutcome::PRESENTATION_FAILURE);

		default:
			return(UIAbortDialogOutcome::PRESENTATION_FAILURE);
	}
}


// Shows the screen through its RmlUi view. FAILED_TO_OPEN leaves nothing shown and the
// caller receives an explicit presentation failure.
UIResult UI_Abort_Screen(UIAbortPresenterClass & presenter);
