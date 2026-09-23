/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>

#include "cursorstate.hh"
#include "pointerscrollpolicy.hh"

namespace
{

constexpr int SCROLL_RATE_SETTING_COUNT = 7;
int Failures = 0;


void Check(bool condition, char const * what)
{
	std::printf("%-74s %s\n", what, condition ? "ok" : "FAILED");
	if (!condition) Failures++;
}


void Test_Action_Cursor_Resolution(void)
{
	Check(Action_Cursor_Shape(ACTION_NONE, false, false, false) == MOUSE_NORMAL,
		"neutral menus and empty tactical ground use the ordinary arrow");
	Check(Action_Cursor_Shape(ACTION_SELECT, false, false, false) == MOUSE_CAN_SELECT
		&& Action_Cursor_Shape(ACTION_TOGGLE_SELECT, false, false, false) == MOUSE_CAN_SELECT,
		"native friendly selection actions use the selection cursor");
	Check(Action_Cursor_Shape(ACTION_MOVE, false, false, false) == MOUSE_CAN_MOVE,
		"native valid movement uses the move cursor");
	Check(Action_Cursor_Shape(ACTION_NOMOVE, false, false, false) == MOUSE_NO_MOVE,
		"native invalid movement uses the no-move cursor");
	Check(Action_Cursor_Shape(ACTION_ATTACK, false, false, true) == MOUSE_STAY_ATTACK
		&& Action_Cursor_Shape(ACTION_ATTACK, false, false, false) == MOUSE_CAN_ATTACK,
		"native enemy attack actions keep their range-specific attack cursor");
	Check(Action_Cursor_Shape(ACTION_MOVE, false, false, false) == MOUSE_CAN_MOVE,
		"a selected MCV over ordinary move ground is not forced into deploy mode");
	Check(Action_Cursor_Shape(ACTION_SELF, false, false, false) == MOUSE_DEPLOY
		&& Action_Cursor_Shape(ACTION_NO_DEPLOY, false, false, false) == MOUSE_NO_DEPLOY,
		"deploy cursors appear only for native self or no-deploy actions");
	Check(Action_Cursor_Shape(ACTION_NOMOVE, true, true, false) == MOUSE_CAN_MOVE,
		"native move-to-shroud units retain their valid hidden-ground cursor");
}


void Test_Repair_Sell_Seeds_And_Hover(void)
{
	Check(RepairSell_Mode_Cursor_Seed(RepairSellModeCursor::Repair) == MOUSE_REPAIR
		&& Action_Cursor_Shape(ACTION_REPAIR, false, false, false) == MOUSE_REPAIR
		&& Action_Cursor_Shape(ACTION_GREPAIR, false, false, false) == MOUSE_GREPAIR,
		"Repair seeds immediately and native repair hover selects the matching valid cursor");
	Check(Action_Cursor_Shape(ACTION_NO_REPAIR, false, false, false) == MOUSE_NO_REPAIR
		&& Action_Cursor_Shape(ACTION_NO_GREPAIR, false, false, false) == MOUSE_NO_REPAIR,
		"invalid repair hover selects the native no-repair cursor");
	Check(RepairSell_Mode_Cursor_Seed(RepairSellModeCursor::Sell) == MOUSE_SELL_BACK
		&& Action_Cursor_Shape(ACTION_SELL, false, false, false) == MOUSE_SELL_BACK
		&& Action_Cursor_Shape(ACTION_SELL_UNIT, false, false, false) == MOUSE_SELL_UNIT,
		"Sell seeds immediately and native hover distinguishes structures from units");
	Check(Action_Cursor_Shape(ACTION_NO_SELL, false, false, false) == MOUSE_NO_SELL_BACK,
		"invalid sell hover selects the native no-sell cursor");
	Check(RepairSell_Mode_Cursor_Seed(RepairSellModeCursor::Normal) == MOUSE_NORMAL,
		"cancelling Repair or Sell seeds the ordinary native pointer state");
}


void Test_Square_Mode_Cycle(void)
{
	RepairSellModeCursor mode = RepairSellModeCursor::Normal;
	mode = Next_RepairSell_Mode_Cursor(mode);
	Check(mode == RepairSellModeCursor::Repair
		&& RepairSell_Mode_Cursor_Seed(mode) == MOUSE_REPAIR,
		"Square arms Repair and seeds the repair cursor");
	mode = Next_RepairSell_Mode_Cursor(mode);
	Check(mode == RepairSellModeCursor::Sell
		&& RepairSell_Mode_Cursor_Seed(mode) == MOUSE_SELL_BACK,
		"Square advances Repair to Sell and seeds the sell cursor");
	mode = Next_RepairSell_Mode_Cursor(mode);
	Check(mode == RepairSellModeCursor::Normal
		&& RepairSell_Mode_Cursor_Seed(mode) == MOUSE_NORMAL,
		"Square advances Sell to Normal and seeds the ordinary cursor");
}


void Test_Mode_Icon_Ownership(void)
{
	std::ifstream file(OPENTS_UI_MODE_ICON_SOURCE);
	std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::size_t const service = source.find("void UI_Mode_Icon_Service(void)");
	std::size_t const end = source.find("void UI_Mode_Icon_Shutdown(void)", service);
	std::string const service_body = service == std::string::npos || end == std::string::npos
		? std::string()
		: source.substr(service, end - service);
	Check(!service_body.empty()
		&& service_body.find("Set_Default_Mouse") == std::string::npos
		&& service_body.find("Apply_Armed_Pointer_Fallback") == std::string::npos,
		"mode-icon service cannot overwrite a target-specific native hover cursor");
	Check(source.find("Can_Deploy_Now") == std::string::npos,
		"selected deployable units are not treated as a persistent deploy mode");
}


void Test_Controller_ScrollRate(void)
{
	double const fastest = Controller_ScrollRate_Scale(0, SCROLL_RATE_SETTING_COUNT);
	double const default_rate = Controller_ScrollRate_Scale(3, SCROLL_RATE_SETTING_COUNT);
	double const slowest = Controller_ScrollRate_Scale(6, SCROLL_RATE_SETTING_COUNT);
	Check(fastest == 1.0 && default_rate == 0.25 && std::abs(slowest - (1.0 / 7.0)) < 0.000001
		&& fastest > default_rate && default_rate > slowest,
		"right-stick speed follows native one-over-setting-plus-one scroll semantics");
	Check(Controller_ScrollRate_Scale(-1, SCROLL_RATE_SETTING_COUNT) == fastest
		&& Controller_ScrollRate_Scale(8, SCROLL_RATE_SETTING_COUNT) == slowest,
		"controller speed remains bounded outside the slider's ScrollRate values");

	int touch_travel[3] = {};
	int const rates[] = {0, 3, 6};
	double controller_scales[3] = {};
	for (int rate_index = 0; rate_index < 3; rate_index++) {
		controller_scales[rate_index] = Controller_ScrollRate_Scale(rates[rate_index],
			SCROLL_RATE_SETTING_COUNT);
		double remainder = 0.0;
		for (int index = 0; index < 10; index++) {
			touch_travel[rate_index] += Scale_Touch_Scroll_Offset(1, 0.5, remainder);
		}
	}
	Check(controller_scales[0] > controller_scales[1] && controller_scales[1] > controller_scales[2]
		&& touch_travel[0] == 5 && touch_travel[1] == touch_travel[0]
		&& touch_travel[2] == touch_travel[0],
		"touch pan retains screen scaling, subpixel travel, and independence from ScrollRate");

	double controller_remainder = 0.0;
	int controller_travel = 0;
	for (int index = 0; index < 10; index++) {
		controller_travel += Scale_Controller_Scroll_Offset(1, 0.5, 3,
			SCROLL_RATE_SETTING_COUNT, controller_remainder);
	}
	Check(controller_travel == 1 && std::abs(controller_remainder - 0.25) < 0.000001,
		"controller scroll-rate scaling retains fractional travel at the current screen scale");
}

}


int main(void)
{
	Test_Action_Cursor_Resolution();
	Test_Repair_Sell_Seeds_And_Hover();
	Test_Square_Mode_Cycle();
	Test_Mode_Icon_Ownership();
	Test_Controller_ScrollRate();
	std::printf("%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
