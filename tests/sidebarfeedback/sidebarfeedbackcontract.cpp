/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Pins the sidebar button projection to Display's authoritative native mode flags.

#include "sidebarfeedback.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

namespace {

int Failures = 0;

void Check(bool condition, char const * name)
{
	std::printf("%-68s %s\n", name, condition ? "ok" : "FAILED");
	if (!condition) Failures++;
}

bool Has(std::string const & text, char const * snippet)
{
	return(text.find(snippet) != std::string::npos);
}

}

int main(void)
{
	RepairSellSidebarFeedbackType const normal = RepairSell_Sidebar_Feedback_From_Native_Flags(false, false);
	RepairSellSidebarFeedbackType const repair = RepairSell_Sidebar_Feedback_From_Native_Flags(true, false);
	RepairSellSidebarFeedbackType const sell = RepairSell_Sidebar_Feedback_From_Native_Flags(false, true);
	Check(!normal.RepairOn && !normal.SellOn, "Normal leaves Repair and Sell unpressed");
	Check(repair.RepairOn && !repair.SellOn, "Normal to Repair presses only Repair");
	Check(!sell.RepairOn && sell.SellOn, "Repair to Sell presses only Sell");
	Check(!RepairSell_Sidebar_Feedback_From_Native_Flags(false, false).RepairOn
		&& !RepairSell_Sidebar_Feedback_From_Native_Flags(false, false).SellOn,
		"Sell to Normal and native cancellation clear both pressed states");

	std::ifstream sidebar_file(OPENTS_SIDEBAR_SOURCE);
	std::string const source((std::istreambuf_iterator<char>(sidebar_file)), std::istreambuf_iterator<char>());
	std::size_t const ai_start = source.find("void SidebarClass::AI(KeyNumType & input, Point2D const & xy)");
	std::size_t const ai_end = source.find("SidebarClass::Recalc", ai_start);
	std::string const ai = ai_start == std::string::npos || ai_end == std::string::npos
		? std::string() : source.substr(ai_start, ai_end - ai_start);
	Check(!ai.empty()
		&& Has(ai, "RepairSell_Sidebar_Feedback_From_Native_Flags(IsRepairMode, IsSellMode)")
		&& Has(ai, "if (repair_sell_feedback.RepairOn) Repair.Turn_On();")
		&& Has(ai, "else Repair.Turn_Off();")
		&& Has(ai, "if (repair_sell_feedback.SellOn) Upgrade.Turn_On();")
		&& Has(ai, "else Upgrade.Turn_Off();"),
		"the existing buttons synchronize in both directions from native mode flags");
	Check(!ai.empty()
		&& Has(ai, "Repair_Mode_Control(-1);")
		&& Has(ai, "Sell_Mode_Control(-1);")
		&& Has(ai, "input == (BUTTON_REPAIR|KN_BUTTON)")
		&& Has(ai, "input == (BUTTON_UPGRADE|KN_BUTTON)"),
		"touch and cursor activation still call the same native Repair and Sell handlers");
	Check(!ai.empty()
		&& ai.find("RepairSell_Sidebar_Feedback_From_Native_Flags")
			> ai.find("Controller_Repair_Sell_Cycle();")
		&& ai.find("RepairSell_Sidebar_Feedback_From_Native_Flags")
			> ai.find("Sell_Mode_Control(-1);"),
		"Square and Circle cancellation are reflected by the next sidebar feedback pass");

	std::printf("\n%s\n", Failures == 0 ? "PASSED" : "FAILED");
	return(Failures == 0 ? 0 : 1);
}
