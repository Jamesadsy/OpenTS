/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Portable, owner-data-free proof of the result contracts at the presentation/gameplay
// boundary. It includes only the toolkit-free presenter headers and never starts the engine.

#include <cstdio>

#include "ui/uiabort.h"
#include "ui/uicampaign.h"
#include "ui/uimainmenu.h"

namespace {

int Failures = 0;


void Check(bool condition, char const * what)
{
	std::printf("%-72s %s\n", what, condition ? "ok" : "FAILED");

	if (!condition) {
		Failures++;
	}
}


UIResult Result(UIResult::OutcomeType outcome)
{
	UIResult result;
	result.Outcome = outcome;
	return(result);
}

}


int main(void)
{
	/*
	 * Campaign acceptance carries a stable identity and is the only result allowed to
	 * commit the selected difficulty.
	 */
	int difficulty = 1;
	UICampaignSelectionResult accepted = UI_Campaign_Selection_Result(
		Result(UIResult::OUTCOME_ACCEPTED), 7, 2);
	Check(accepted.Outcome == UICampaignSelectionOutcome::ACCEPTED && accepted.Campaign == 7,
		"campaign accept carries a stable selected identity");
	UI_Campaign_Commit_Difficulty(accepted, difficulty);
	Check(difficulty == 2, "campaign difficulty commits on accept");

	UICampaignSelectionResult cancelled = UI_Campaign_Selection_Result(
		Result(UIResult::OUTCOME_CANCELLED), -1, 0);
	difficulty = 1;
	UI_Campaign_Commit_Difficulty(cancelled, difficulty);
	Check(cancelled.Outcome == UICampaignSelectionOutcome::CANCELLED,
		"campaign cancel is distinguishable");
	Check(difficulty == 1, "campaign cancel leaves difficulty unchanged");

	UICampaignSelectionResult failed = UI_Campaign_Selection_Result(
		Result(UIResult::OUTCOME_FAILED_TO_OPEN), -1, 2);
	UI_Campaign_Commit_Difficulty(failed, difficulty);
	Check(failed.Outcome == UICampaignSelectionOutcome::PRESENTATION_FAILURE,
		"campaign failed-to-open is distinguishable from cancel");
	Check(failed.Campaign < 0 && difficulty == 1,
		"campaign failed-to-open is not CAMPAIGN_NONE cancellation or a difficulty commit");

	/*
	 * Abort choices preserve their gameplay meaning, while a missing presenter is explicit.
	 */
	Check(UI_Abort_Dialog_Result(Result(UIResult::OUTCOME_ACCEPTED),
		UIAbortPresenterClass::CHOICE_QUIT) == UIAbortDialogOutcome::QUIT,
		"abort quit is distinguishable");
	Check(UI_Abort_Dialog_Result(Result(UIResult::OUTCOME_ACCEPTED),
		UIAbortPresenterClass::CHOICE_RESTART) == UIAbortDialogOutcome::RESTART_OR_SURRENDER,
		"abort restart/surrender is distinguishable");
	Check(UI_Abort_Dialog_Result(Result(UIResult::OUTCOME_CANCELLED),
		UIAbortPresenterClass::CHOICE_CANCEL) == UIAbortDialogOutcome::CANCEL,
		"abort cancel is distinguishable");
	UIAbortDialogOutcome const abort_failure = UI_Abort_Dialog_Result(
		Result(UIResult::OUTCOME_FAILED_TO_OPEN), UIAbortPresenterClass::CHOICE_NONE);
	Check(abort_failure == UIAbortDialogOutcome::PRESENTATION_FAILURE,
		"abort failed-to-open is distinguishable");
	Check(abort_failure != UIAbortDialogOutcome::CANCEL,
		"abort failed-to-open is not default zero/cancel/no-action");

	/* The graphical NewMenu fallback remains the donor RmlUi Main_Menu path. */
	Check(UI_MAINMENU_FALLBACK_ROUTE == UIMainMenuFallbackRoute::DONOR_RMLUI_MAIN_MENU,
		"NewMenu fallback selects the donor RmlUi Main_Menu contract");
	Check(!UI_MAINMENU_LEGACY_OWNERDRAW_FALLBACK,
		"the presentation fallback does not resurrect OwnerDraw/winfix");

	std::printf("\n%s\n", Failures == 0 ? "PASSED" : "FAILED");
	return(Failures == 0 ? 0 : 1);
}
