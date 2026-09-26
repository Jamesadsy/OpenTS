/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include <cstdio>
#include <cmath>
#include <array>

#include "controller_test.h"
#include "win32compat.h"

namespace
{

constexpr Uint64 SECOND = 1000000000ULL;
constexpr Uint64 FRAME_60 = SECOND / 60;
constexpr Uint64 FRAME_30 = SECOND / 30;
int Failures = 0;


void Check(bool condition, char const * what)
{
	std::printf("%-74s %s\n", what, condition ? "ok" : "FAILED");
	if (!condition) Failures++;
}


void Drain_Key_Events(void)
{
	int virtualkey = 0;
	bool down = false;
	while (Win32_Gamepad_Test_Take_Key_Event(&virtualkey, &down)) {}
}


bool Take_Key(int expected, bool expected_down)
{
	int virtualkey = 0;
	bool down = false;
	return(Win32_Gamepad_Test_Take_Key_Event(&virtualkey, &down)
		&& virtualkey == expected && down == expected_down);
}


bool Take_Mouse(Uint8 expected_button, bool expected_down, float expected_x, float expected_y)
{
	Uint8 button = 0;
	bool down = false;
	float x = 0.0f;
	float y = 0.0f;
	return(Win32_Gamepad_Test_Take_Mouse_Event(&button, &down, &x, &y)
		&& button == expected_button && down == expected_down
		&& x == expected_x && y == expected_y);
}


void Drain_Mouse_Events(void)
{
	Uint8 button = 0;
	bool down = false;
	float x = 0.0f;
	float y = 0.0f;
	while (Win32_Gamepad_Test_Take_Mouse_Event(&button, &down, &x, &y)) {}
}


void Service(Uint64 now)
{
	Win32_Gamepad_Test_Set_Now(now);
	Win32_Gamepad_Test_Service();
}


void Test_Cold_Boot_Menu_Pointer(void)
{
	float x = -1.0f;
	float y = -1.0f;
	Win32_Pointer_Position(&x, &y);
	Check(x == 0.0f && y == 0.0f,
		"cold-boot controller proof begins with no gameplay-seeded pointer position");

	Win32_Gamepad_Test_Reset();
	Win32_Gamepad_Test_Set_Window_Size(640.0f, 480.0f);
	Win32_Gamepad_Test_Set_Connected(true);
	Win32_Gamepad_Set_Menu_Surface(true);
	Check(!Win32_Gamepad_Menu_Focus_Owned() && Win32_Pointer_Is_Drawn(),
		"the first interactive frontend begins in the existing Pointer Mode");

	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, -32768);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTY, -32768);
	Service(0);
	Win32_Pointer_Position(&x, &y);
	Check(!Win32_Gamepad_Menu_Focus_Owned() && !Win32_Pointer_Menu_Focus()
		&& Win32_Pointer_Is_Drawn() && Win32_Pointer_Is_Controller_Owner()
		&& x == 319.5f && y == 239.5f,
		"first cold-boot stick input immediately enters Pointer Mode at a live viewport-derived position");
	Service(FRAME_60);
	Win32_Pointer_Position(&x, &y);
	Check(x > 0.0f && y > 0.0f && x < 319.5f && y < 239.5f,
		"the first controller pointer moves correctly from its cold-boot position");

	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, true);
	Check(Take_Mouse(SDL_BUTTON_LEFT, true, x, y),
		"cold-boot Cross presses the current authoritative pointer coordinate");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, false);
	Check(Take_Mouse(SDL_BUTTON_LEFT, false, x, y),
		"cold-boot Cross releases at the same pointer coordinate");

	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true);
	Check(Win32_Gamepad_Menu_Focus_Owned() && !Win32_Pointer_Is_Drawn(),
		"D-pad enters Focus Mode after cold-boot pointer movement");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, false);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 12000);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTY, 0);
	Service(2 * FRAME_60);
	Win32_Pointer_Position(&x, &y);
	Check(!Win32_Gamepad_Menu_Focus_Owned() && Win32_Pointer_Is_Drawn()
		&& x > 0.0f && y > 0.0f,
		"D-pad to stick switching remains deterministic at the current pointer position");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_DOWN, true);
	Check(Win32_Gamepad_Menu_Focus_Owned() && !Win32_Pointer_Is_Drawn(),
		"D-pad returns to Focus Mode after another stick movement");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_DOWN, false);

	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 0);
	Win32_Gamepad_Set_Menu_Surface(false);
	Win32_Pointer_Move(500.0f, 350.0f);
	Win32_Gamepad_Set_Menu_Surface(true);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_DOWN, true);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_DOWN, false);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 12000);
	Service(3 * FRAME_60);
	Win32_Pointer_Position(&x, &y);
	Check(!Win32_Gamepad_Menu_Focus_Owned() && Win32_Pointer_Is_Drawn()
		&& x > 500.0f && y == 350.0f,
		"a gameplay-returned frontend preserves its established pointer and the same D-pad-to-stick behavior");
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 0);
	Win32_Gamepad_Set_Menu_Surface(false);
}


void Setup(void)
{
	Win32_Gamepad_Test_Reset();
	Win32_Gamepad_Test_Set_Window_Size(640.0f, 480.0f);
	Win32_Pointer_Move(320.0f, 240.0f);
	Win32_Gamepad_Test_Set_Connected(true);
	Service(0);
	Drain_Key_Events();
}


void Test_Stick_Dead_Zone_And_Bounds(void)
{
	Setup();
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 3999);
	Service(FRAME_60);
	float x = 0.0f;
	float y = 0.0f;
	Win32_Pointer_Position(&x, &y);
	Check(x == 320.0f && y == 240.0f, "left-stick dead zone leaves the effective pointer still");

	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 10000);
	Service(2 * FRAME_60);
	Win32_Pointer_Position(&x, &y);
	Check(x > 320.0f && y == 240.0f && Win32_Pointer_Is_Controller_Owner(),
		"left stick moves the actual pointer with shaped response and claims pointer ownership");

	Win32_Pointer_Move(639.0f, 479.0f);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 32767);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTY, 32767);
	Service(3 * FRAME_60);
	Win32_Pointer_Position(&x, &y);
	Check(x == 639.0f && y == 479.0f, "left stick clamps at the lower-right window bounds");

	Win32_Pointer_Move(0.0f, 0.0f);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, -32768);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTY, -32768);
	Service(4 * FRAME_60);
	Win32_Pointer_Position(&x, &y);
	Check(x == 0.0f && y == 0.0f, "left stick clamps at the upper-left window bounds");
}


void Test_Speed_Boost(void)
{
	Setup();
	Win32_Pointer_Move(100.0f, 100.0f);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 16000);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0);
	Service(FRAME_60);
	float normal_x = 0.0f;
	float ignored = 0.0f;
	Win32_Pointer_Position(&normal_x, &ignored);

	Setup();
	Win32_Pointer_Move(100.0f, 100.0f);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 16000);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 32767);
	Service(FRAME_60);
	float boosted_x = 0.0f;
	Win32_Pointer_Position(&boosted_x, &ignored);
	Check(boosted_x > normal_x, "R2 trigger increases virtual-cursor travel");
}


float One_Second_Travel(Uint64 frame)
{
	Setup();
	Win32_Pointer_Move(100.0f, 100.0f);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 32767);
	Service(frame);
	for (int index = 2; index <= (int)(SECOND / frame); index++) {
		Service((Uint64)index * frame);
	}

	float x = 0.0f;
	float y = 0.0f;
	Win32_Pointer_Position(&x, &y);
	return(x - 100.0f);
}


void Test_Cadence_And_Stall(void)
{
	float const travel_60 = One_Second_Travel(FRAME_60);
	float const travel_30 = One_Second_Travel(FRAME_30);
	Check(travel_60 > 200.0f && travel_30 > 200.0f
		&& std::abs(travel_60 - travel_30) < 2.0f,
		"60 Hz and 30 Hz service cadences integrate comparable one-second travel");

	Setup();
	Win32_Pointer_Move(100.0f, 100.0f);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 32767);
	Service(FRAME_60);
	float before = 0.0f;
	float ignored = 0.0f;
	Win32_Pointer_Position(&before, &ignored);
	Service(FRAME_60 + SECOND / 4);
	float after = 0.0f;
	Win32_Pointer_Position(&after, &ignored);
	Check(after - before < 2.0f,
		"a delayed controller service interval is discarded instead of becoming a catch-up teleport");
}


void Test_Action_Edges(void)
{
	Setup();
	int action = 0;
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_WEST, true);
	Check(Win32_Gamepad_Take_Action(&action) && action == WIN32_GAMEPAD_ACTION_SQUARE,
		"Square produces one explicit game action edge");
	Check(!Win32_Gamepad_Take_Action(&action),
		"holding Square does not auto-repeat the game action");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_WEST, false);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_WEST, true);
	Check(Win32_Gamepad_Take_Action(&action) && action == WIN32_GAMEPAD_ACTION_SQUARE,
		"a second Square press produces the next action edge");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_WEST, false);

	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_NORTH, true);
	Check(Win32_Gamepad_Take_Action(&action) && action == WIN32_GAMEPAD_ACTION_TRIANGLE,
		"Triangle produces one explicit game action edge");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_NORTH, false);
	Win32_Gamepad_Discard_Actions();
	Check(!Win32_Gamepad_Take_Action(&action),
		"frontend/action teardown can discard pending gameplay edges");
}


void Test_Face_Buttons_And_Modifiers(void)
{
	Setup();
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, true);
	Check((Win32_Pointer_Buttons() & SDL_BUTTON_LMASK) != 0,
		"Cross/A holds the effective left mouse button");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, true);
	Check((Win32_Pointer_Buttons() & SDL_BUTTON_LMASK) != 0,
		"a held Cross/A does not create another left-button edge");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, false);
	Check((Win32_Pointer_Buttons() & SDL_BUTTON_LMASK) == 0,
		"Cross/A release clears only the controller left-button source");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, false);
	Check((Win32_Pointer_Buttons() & SDL_BUTTON_LMASK) == 0,
		"a repeated Cross/A release does not create another left-button edge");

	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_EAST, true);
	Check((Win32_Pointer_Buttons() & SDL_BUTTON_RMASK) != 0,
		"Circle/B holds the effective right mouse button");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_EAST, false);
	Check((Win32_Pointer_Buttons() & SDL_BUTTON_RMASK) == 0,
		"Circle/B release clears the effective right mouse button");

	Win32_Pointer_Button(SDL_BUTTON_LEFT, true);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, true);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, false);
	Check((Win32_Pointer_Buttons() & SDL_BUTTON_LMASK) != 0,
		"controller release does not release a separately held host pointer button");
	Win32_Pointer_Button(SDL_BUTTON_LEFT, false);

	Drain_Key_Events();
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true);
	Check((GetAsyncKeyState(WIN32_VK_CONTROL) & 0x8000) != 0,
		"L1 exposes a held synthetic Control modifier to polling code");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, false);
	Check((GetAsyncKeyState(WIN32_VK_CONTROL) & 0x8000) == 0,
		"L1 release clears the synthetic Control modifier");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, true);
	Check((GetAsyncKeyState(WIN32_VK_MENU) & 0x8000) != 0,
		"R1 exposes a held synthetic Alt modifier to polling code");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, false);
	Check((GetAsyncKeyState(WIN32_VK_MENU) & 0x8000) == 0,
		"R1 release clears the synthetic Alt modifier");
	Drain_Key_Events();
}


void Test_Menu_Focus_And_Pointer_Ownership(void)
{
	Setup();
	Win32_Gamepad_Set_Menu_Surface(true);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_DOWN, true);
	Check(Win32_Gamepad_Menu_Focus_Owned() && Win32_Pointer_Menu_Focus()
		&& !Win32_Pointer_Is_Drawn(),
		"D-pad navigation transfers menu ownership to visible focus and hides the pointer");
	Check(Take_Key(WIN32_VK_DOWN, true), "focus-mode D-pad continues to deliver Down to the active screen");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_DOWN, false);
	Check(Take_Key(WIN32_VK_DOWN, false), "focus-mode D-pad emits the matching Down release");

	Drain_Key_Events();
	Drain_Mouse_Events();
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, true);
	Check(Take_Key(WIN32_VK_RETURN, true)
		&& (Win32_Pointer_Buttons() & SDL_BUTTON_LMASK) == 0
		&& !Win32_Gamepad_Test_Take_Mouse_Event(NULL, NULL, NULL, NULL),
		"Cross in Focus Mode emits semantic Enter with no stale-coordinate mouse activation");

	Win32_Pointer_Move(100.0f, 120.0f);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 10000);
	Service(FRAME_60);
	float pointer_x = 0.0f;
	float pointer_y = 0.0f;
	Win32_Pointer_Position(&pointer_x, &pointer_y);
	Check(!Win32_Gamepad_Menu_Focus_Owned() && !Win32_Pointer_Menu_Focus()
		&& Win32_Pointer_Is_Drawn() && pointer_x > 100.0f && pointer_y == 120.0f,
		"meaningful left-stick motion immediately restores authoritative pointer mode");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, false);
	Check(Take_Key(WIN32_VK_RETURN, false)
		&& !Win32_Gamepad_Test_Take_Mouse_Event(NULL, NULL, NULL, NULL),
		"a Cross press begun in Focus Mode retains semantic release after a stick switch");

	Drain_Key_Events();
	Drain_Mouse_Events();
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, true);
	Check(!Win32_Gamepad_Test_Take_Key_Event(NULL, NULL)
		&& Take_Mouse(SDL_BUTTON_LEFT, true, pointer_x, pointer_y),
		"Cross in Pointer Mode presses left mouse at the actual virtual-pointer position");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, false);
	Check(Take_Mouse(SDL_BUTTON_LEFT, false, pointer_x, pointer_y),
		"pointer-mode Cross releases left mouse at the actual virtual-pointer position");

	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true);
	Check(Win32_Gamepad_Menu_Focus_Owned(), "D-pad to stick to D-pad returns to Focus Mode");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, false);
	Drain_Key_Events();
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_EAST, true);
	Check(Win32_Gamepad_Take_Menu_Back_Action()
		&& !Win32_Gamepad_Take_Menu_Back_Action()
		&& !Win32_Gamepad_Test_Take_Key_Event(NULL, NULL)
		&& (Win32_Pointer_Buttons() & SDL_BUTTON_RMASK) == 0,
		"Circle in Focus Mode queues one semantic Back edge without Escape or right-click");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_EAST, false);
	Check(!Win32_Gamepad_Take_Menu_Back_Action()
		&& !Win32_Gamepad_Test_Take_Key_Event(NULL, NULL),
		"focus-mode Circle release does not enqueue another Back or key edge");

	Win32_Gamepad_Menu_Pointer_Moved();
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_EAST, true);
	Check(Take_Mouse(SDL_BUTTON_RIGHT, true, pointer_x, pointer_y),
		"Circle in Pointer Mode preserves right-click at the current pointer position");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_EAST, false);
	Check(Take_Mouse(SDL_BUTTON_RIGHT, false, pointer_x, pointer_y),
		"pointer-mode Circle releases right mouse at the same pointer position");

	Win32_Touch_Set_Movie_Mode(true);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_EAST, true);
	Check(!Win32_Gamepad_Test_Take_Key_Event(NULL, NULL)
		&& !Win32_Gamepad_Test_Take_Mouse_Event(NULL, NULL, NULL, NULL),
		"movie Circle remains held by the movie-skip route instead of UI or pointer input");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_EAST, false);
	Win32_Touch_Set_Movie_Mode(false);

	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_UP, true);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_UP, false);
	Win32_Gamepad_Test_Set_Connected(false);
	Check(!Win32_Gamepad_Menu_Focus_Owned() && !Win32_Pointer_Menu_Focus()
		&& Win32_Pointer_Is_Drawn(),
		"controller disconnect safely clears Focus Mode and restores the pointer presentation");

	Win32_Gamepad_Set_Menu_Surface(false);
	Win32_Gamepad_Test_Set_Connected(true);
	float tactical_x = 0.0f;
	float tactical_y = 0.0f;
	Win32_Pointer_Position(&tactical_x, &tactical_y);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_UP, true);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_UP, false);
	float tactical_x_after = 0.0f;
	float tactical_y_after = 0.0f;
	Win32_Pointer_Position(&tactical_x_after, &tactical_y_after);
	Check(!Win32_Gamepad_Menu_Focus_Owned() && Win32_Pointer_Is_Drawn()
		&& tactical_x_after == tactical_x && tactical_y_after == tactical_y,
		"leaving the menu restores the unchanged tactical pointer presentation");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_EAST, true);
	Check(Take_Mouse(SDL_BUTTON_RIGHT, true, tactical_x, tactical_y),
		"tactical Circle sends right-button down for native Repair/Sell cancellation");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_EAST, false);
	Check(Take_Mouse(SDL_BUTTON_RIGHT, false, tactical_x, tactical_y),
		"tactical Circle release preserves the existing native cancel path");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_WEST, true);
	int action = 0;
	Check(Win32_Gamepad_Take_Action(&action) && action == WIN32_GAMEPAD_ACTION_SQUARE,
		"Square gameplay action remains mapped to the existing Repair/Sell endpoint");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_WEST, false);
}


void Test_Start_And_Dpad(void)
{
	Setup();
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_START, true);
	Check(Take_Key(WIN32_VK_ESCAPE, true) && Take_Key(WIN32_VK_ESCAPE, false),
		"Start emits one Escape press and release");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_START, false);

	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_LEFT, true);
	Check(Take_Key(WIN32_VK_LEFT, true), "D-pad left emits a native left navigation key");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_LEFT, false);
	Check(Take_Key(WIN32_VK_LEFT, false), "D-pad left emits a matching native key release");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, true);
	int key = 0;
	bool down = false;
	Check(Win32_Gamepad_Test_Take_Key_Event(&key, &down) && key == WIN32_VK_RIGHT && down,
		"D-pad right stays a navigation key rather than printable text");
	Check(key != '4' && key != '2', "D-pad navigation never injects printable 4 or 2");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_RIGHT, false);
	Drain_Key_Events();

	float pointer_x = 0.0f;
	float pointer_y = 0.0f;
	Win32_Pointer_Position(&pointer_x, &pointer_y);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_UP, true);
	Check(Take_Key(WIN32_VK_UP, true), "D-pad up remains a native key for the active screen");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_UP, false);
	Check(Take_Key(WIN32_VK_UP, false), "D-pad up emits a matching native key release");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_DOWN, true);
	Check(Take_Key(WIN32_VK_DOWN, true), "D-pad down remains a native key for the active screen");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_DOWN, false);
	Check(Take_Key(WIN32_VK_DOWN, false), "D-pad down emits a matching native key release");
	float pointer_x_after = 0.0f;
	float pointer_y_after = 0.0f;
	Win32_Pointer_Position(&pointer_x_after, &pointer_y_after);
	Check(pointer_x_after == pointer_x && pointer_y_after == pointer_y,
		"D-pad list keys do not move the tactical pointer");
}


void Test_Right_Stick_Camera_Pan(void)
{
	Setup();
	Win32_Pointer_Move(250.0f, 200.0f);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTX, 32767);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTY, 32767);
	Service(FRAME_60);
	float x = 0.0f;
	float y = 0.0f;
	Win32_Pointer_Position(&x, &y);
	int panx = 0;
	int pany = 0;
	Check(x == 250.0f && y == 200.0f, "right stick leaves the visible cursor position unchanged");
	Check(Win32_Gamepad_Take_Camera_Pan(&panx, &pany) && panx > 0 && pany > 0
		&& !Win32_Pointer_Is_Controller_Owner(),
		"right stick produces diagonal native camera-pan output");

	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTX, -32768);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTY, 32767);
	Service(2 * FRAME_60);
	Check(Win32_Gamepad_Take_Camera_Pan(&panx, &pany) && panx < 0 && pany > 0,
		"right-stick direction changes preserve independent x and y pan");

	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTX, 0);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTY, 0);
	Service(3 * FRAME_60);
	Check(!Win32_Gamepad_Take_Camera_Pan(&panx, &pany),
		"neutral right stick produces no further camera movement");
}


void Test_Left_Stick_Edge_With_Right_Stick_Camera(void)
{
	auto camera_travel = [](bool move_pointer) {
		Setup();
		if (move_pointer) {
			Win32_Pointer_Move(600.0f, 240.0f);
			Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 32767);
		}
		Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTX, 16000);
		Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTY, -16000);
		Service(FRAME_60);
		for (int frame = 2; frame <= 40; frame++) {
			Service((Uint64)frame * FRAME_60);
		}
		int panx = 0;
		int pany = 0;
		Win32_Gamepad_Take_Camera_Pan(&panx, &pany);
		float pointer_x = 0.0f;
		float pointer_y = 0.0f;
		Win32_Pointer_Position(&pointer_x, &pointer_y);
		return(std::array<int, 4>{panx, pany, Win32_Pointer_Is_Controller_Owner() ? 1 : 0,
			pointer_x >= 639.0f && pointer_y == 240.0f ? 1 : 0});
	};

	std::array<int, 4> const right_only = camera_travel(false);
	std::array<int, 4> const left_at_edge = camera_travel(true);
	Check(left_at_edge[2] == 1 && left_at_edge[3] == 1
		&& left_at_edge[0] == right_only[0] && left_at_edge[1] == right_only[1],
		"left stick reaches the edge while simultaneous right-stick pan equals the right-only camera input");
}


void Test_Pointer_Ownership_Transitions(void)
{
	Setup();
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 12000);
	Service(FRAME_60);
	Check(Win32_Pointer_Is_Controller_Owner(),
		"meaningful left-stick pointer movement suppresses controller-owned native edge scroll");

	Win32_Pointer_Move_Host(20.0f, 20.0f);
	Check(!Win32_Pointer_Is_Controller_Owner(),
		"real host mouse movement returns pointer ownership to native hardware edge scroll");

	Win32_Pointer_Move_Controller(30.0f, 30.0f);
	Win32_Pointer_Set_Direct_Touch(true);
	Check(!Win32_Pointer_Is_Controller_Owner(),
		"direct touch clears controller pointer ownership");
	Win32_Pointer_Set_Direct_Touch(false);

	Win32_Pointer_Move_Controller(40.0f, 40.0f);
	Win32_Gamepad_Test_Set_Connected(false);
	Check(!Win32_Pointer_Is_Controller_Owner(),
		"controller disconnect clears pointer ownership without retaining edge-scroll suppression");

	Win32_Gamepad_Test_Set_Connected(true);
	Win32_Pointer_Move_Controller(50.0f, 50.0f);
	Win32_Gamepad_Set_Focus(false);
	Check(!Win32_Pointer_Is_Controller_Owner(),
		"background/focus reset clears controller pointer ownership");
}


int One_Second_Right_Stick_Travel(Uint64 frame, Sint16 axis)
{
	Setup();
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTX, axis);
	Service(frame);
	for (int index = 2; index <= (int)(SECOND / frame); index++) {
		Service((Uint64)index * frame);
	}

	int panx = 0;
	int pany = 0;
	Win32_Gamepad_Take_Camera_Pan(&panx, &pany);
	return(panx);
}


void Test_Right_Stick_Time_And_Analog(void)
{
	int const travel_60 = One_Second_Right_Stick_Travel(FRAME_60, 32767);
	int const travel_30 = One_Second_Right_Stick_Travel(FRAME_30, 32767);
	int const partial = One_Second_Right_Stick_Travel(FRAME_60, 16384);
	Check(travel_60 >= 479 && travel_60 <= 480 && travel_30 >= 479 && travel_30 <= 480
		&& std::abs(travel_60 - travel_30) <= 1,
		"right-stick camera pan integrates elapsed time consistently at 60 Hz and 30 Hz");
	Check(partial > 100 && partial < travel_60,
		"right-stick camera pan preserves analog stick magnitude");
}


void Test_Right_Stick_R2_Independence(void)
{
	Setup();
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTX, 20000);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTY, -20000);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0);
	Service(FRAME_60);
	int normal_x = 0;
	int normal_y = 0;
	Win32_Gamepad_Take_Camera_Pan(&normal_x, &normal_y);

	Setup();
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTX, 20000);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTY, -20000);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 32767);
	Service(FRAME_60);
	int boosted_x = 0;
	int boosted_y = 0;
	Win32_Gamepad_Take_Camera_Pan(&boosted_x, &boosted_y);
	Check(normal_x == boosted_x && normal_y == boosted_y,
		"R2 does not alter native right-stick camera-pan amount");
}


void Test_Lifecycle_Release_And_Reconnect(void)
{
	Setup();
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, true);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, true);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_DPAD_UP, true);
	Drain_Key_Events();
	Win32_Gamepad_Set_Focus(false);
	Check(Win32_Pointer_Buttons() == 0 && (GetAsyncKeyState(WIN32_VK_CONTROL) & 0x8000) == 0,
		"focus loss releases held buttons and modifiers");
	Check(Take_Key(WIN32_VK_CONTROL, false) && Take_Key(WIN32_VK_UP, false),
		"focus loss emits matching modifier and D-pad releases");

	Win32_Gamepad_Set_Focus(true);
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, true);
	float before_reconnect_x = 0.0f;
	float before_reconnect_y = 0.0f;
	Win32_Pointer_Position(&before_reconnect_x, &before_reconnect_y);
	Win32_Gamepad_Test_Set_Connected(false);
	Check(Win32_Pointer_Buttons() == 0, "device removal releases a held face button");
	Win32_Gamepad_Test_Set_Connected(true);
	Drain_Key_Events();
	float after_reconnect_x = 0.0f;
	float after_reconnect_y = 0.0f;
	Win32_Pointer_Position(&after_reconnect_x, &after_reconnect_y);
	Check(after_reconnect_x == before_reconnect_x && after_reconnect_y == before_reconnect_y,
		"controller reconnect preserves the pointer state without a centre jump");
}

}


int main(void)
{
	Test_Cold_Boot_Menu_Pointer();
	Test_Stick_Dead_Zone_And_Bounds();
	Test_Speed_Boost();
	Test_Cadence_And_Stall();
	Test_Action_Edges();
	Test_Face_Buttons_And_Modifiers();
	Test_Menu_Focus_And_Pointer_Ownership();
	Test_Start_And_Dpad();
	Test_Right_Stick_Camera_Pan();
	Test_Left_Stick_Edge_With_Right_Stick_Camera();
	Test_Pointer_Ownership_Transitions();
	Test_Right_Stick_Time_And_Analog();
	Test_Right_Stick_R2_Independence();
	Test_Lifecycle_Release_And_Reconnect();
	std::printf("%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
