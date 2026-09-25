/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "controllerownership.h"
#include "ui/uicontrollerfocus.h"

#include <cstdio>

namespace
{

int Failures = 0;


void Check(bool condition, char const * what)
{
	std::printf("%-72s %s\n", what, condition ? "ok" : "FAILED");
	if (!condition) {
		Failures++;
	}
}


void Test_Menu_Navigation_Policies(void)
{
	using UI_Controller_Focus::Direction;
	using UI_Controller_Focus::Next_Target_Index;
	using UI_Controller_Focus::TargetPosition;

	std::vector<TargetPosition> const main_menu = {
		{0.0f, 0.0f}, {0.0f, 30.0f}, {0.0f, 60.0f},
		{0.0f, 90.0f}, {0.0f, 120.0f}, {0.0f, 150.0f}
	};
	Check(Next_Target_Index(main_menu, -1, Direction::DOWN) == 0
		&& Next_Target_Index(main_menu, 0, Direction::DOWN) == 1
		&& Next_Target_Index(main_menu, 5, Direction::DOWN) == 0
		&& Next_Target_Index(main_menu, 0, Direction::UP) == 5,
		"main-menu Up/Down follows document order and wraps through the valid actions");

	std::vector<TargetPosition> const options_menu = {
		{0.0f, 0.0f}, {0.0f, 30.0f}, {0.0f, 60.0f}, {0.0f, 90.0f}, {0.0f, 120.0f}
	};
	Check(Next_Target_Index(options_menu, 1, Direction::DOWN) == 2
		&& Next_Target_Index(options_menu, 0, Direction::UP) == 4,
		"frontend options menu uses the same shared wrapped focus order");

	std::vector<TargetPosition> const pause_menu = {
		{0.0f, 0.0f}, {0.0f, 30.0f}, {0.0f, 60.0f}, {0.0f, 90.0f},
		{0.0f, 120.0f}, {0.0f, 150.0f}, {0.0f, 180.0f}
	};
	Check(Next_Target_Index(pause_menu, 5, Direction::DOWN) == 6
		&& Next_Target_Index(pause_menu, 6, Direction::DOWN) == 0,
		"pause/options menu shares the same wrapped document-order navigation");

	std::vector<TargetPosition> const horizontal_choices = {
		{10.0f, 20.0f}, {90.0f, 22.0f}, {190.0f, 21.0f}, {95.0f, 100.0f}
	};
	Check(Next_Target_Index(horizontal_choices, 0, Direction::RIGHT) == 1
		&& Next_Target_Index(horizontal_choices, 1, Direction::RIGHT) == 2
		&& Next_Target_Index(horizontal_choices, 2, Direction::LEFT) == 1,
		"Left/Right selects the nearest naturally aligned control in its direction");
}

}


int main(void)
{
	Test_Menu_Navigation_Policies();
	ControllerMenuOwnershipModel ownership;
	ownership.Set_Menu_Surface(true);
	Check(ownership.Mode() == ControllerMenuMode::POINTER,
		"a newly opened menu starts with pointer ownership");

	ownership.Dpad_Navigated();
	Check(ownership.Focus_Owns_Menu()
		&& ownership.Cross_Route() == ControllerMenuRoute::FOCUS
		&& ownership.Circle_Route(false) == ControllerMenuRoute::FOCUS,
		"D-pad navigation assigns both menu actions to focus mode");

	int activations = 0;
	bool const focused_activation = UI_Controller_Focus::Activate_Focused_Action(
		true, false, true, false, [&activations]() { activations++; });
	Check(focused_activation && activations == 1,
		"Cross semantic activation invokes the focused UI action exactly once");
	bool const disabled_activation = UI_Controller_Focus::Activate_Focused_Action(
		true, false, true, true, [&activations]() { activations++; });
	Check(disabled_activation && activations == 1,
		"a disabled focused action is consumed without invoking another UI target");
	bool const pointer_activation = UI_Controller_Focus::Activate_Focused_Action(
		false, false, true, false, [&activations]() { activations++; });
	Check(!pointer_activation && activations == 1,
		"Pointer Mode leaves Cross to its pointer-click route without activating focus");
	bool const screen_owned_activation = UI_Controller_Focus::Activate_Focused_Action(
		true, true, true, false, [&activations]() { activations++; });
	Check(!screen_owned_activation && activations == 1,
		"screen-owned navigation keeps its explicit action path");

	ownership.Pointer_Moved();
	Check(!ownership.Focus_Owns_Menu()
		&& ownership.Cross_Route() == ControllerMenuRoute::POINTER
		&& ownership.Circle_Route(false) == ControllerMenuRoute::POINTER,
		"left-stick movement restores pointer-mode menu actions");

	ownership.Dpad_Navigated();
	Check(ownership.Cross_Route() == ControllerMenuRoute::FOCUS,
		"D-pad to stick to D-pad switching follows the latest input");

	ControllerMenuRoute const pressed_route = ownership.Cross_Route();
	ownership.Pointer_Moved();
	Check(pressed_route == ControllerMenuRoute::FOCUS
		&& ownership.Cross_Route() == ControllerMenuRoute::POINTER,
		"a pressed face button can retain its route while ownership changes");

	ownership.Dpad_Navigated();
	Check(ownership.Circle_Route(true) == ControllerMenuRoute::MOVIE,
		"movie mode keeps Circle on the existing hold-to-skip route");

	ownership.Reset();
	Check(ownership.Menu_Surface_Active()
		&& ownership.Mode() == ControllerMenuMode::POINTER,
		"controller reset clears ownership without closing the current menu scope");

	ownership.Dpad_Navigated();
	ownership.Set_Menu_Surface(false);
	Check(!ownership.Menu_Surface_Active() && !ownership.Focus_Owns_Menu()
		&& ownership.Cross_Route() == ControllerMenuRoute::POINTER,
		"leaving the menu clears focus ownership before gameplay input resumes");

	std::printf("\n%s\n", Failures == 0 ? "PASSED" : "FAILED");
	return(Failures == 0 ? 0 : 1);
}
