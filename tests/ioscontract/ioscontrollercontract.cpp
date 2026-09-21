/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include <cstdio>

#include "controller_test.h"
#include "win32compat.h"

namespace
{

constexpr Uint64 SECOND = 1000000000ULL;
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


void Service(Uint64 now)
{
	Win32_Gamepad_Test_Set_Now(now);
	Win32_Gamepad_Test_Service();
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
	Service(SECOND);
	float x = 0.0f;
	float y = 0.0f;
	Win32_Pointer_Position(&x, &y);
	Check(x == 320.0f && y == 240.0f, "left-stick dead zone leaves the effective pointer still");

	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 10000);
	Service(2 * SECOND);
	Win32_Pointer_Position(&x, &y);
	Check(x > 320.0f && y == 240.0f, "left stick moves the actual pointer with shaped response");

	Win32_Pointer_Move(639.0f, 479.0f);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 32767);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTY, 32767);
	Service(3 * SECOND);
	Win32_Pointer_Position(&x, &y);
	Check(x == 639.0f && y == 479.0f, "left stick clamps at the lower-right window bounds");

	Win32_Pointer_Move(0.0f, 0.0f);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, -32768);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTY, -32768);
	Service(4 * SECOND);
	Win32_Pointer_Position(&x, &y);
	Check(x == 0.0f && y == 0.0f, "left stick clamps at the upper-left window bounds");
}


void Test_Speed_Boost(void)
{
	Setup();
	Win32_Pointer_Move(100.0f, 100.0f);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 16000);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0);
	Service(SECOND);
	float normal_x = 0.0f;
	float ignored = 0.0f;
	Win32_Pointer_Position(&normal_x, &ignored);

	Setup();
	Win32_Pointer_Move(100.0f, 100.0f);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_LEFTX, 16000);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 32767);
	Service(SECOND);
	float boosted_x = 0.0f;
	Win32_Pointer_Position(&boosted_x, &ignored);
	Check(boosted_x > normal_x, "R2 trigger increases virtual-cursor travel");
}


void Test_Face_Buttons_And_Modifiers(void)
{
	Setup();
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, true);
	Check((Win32_Pointer_Buttons() & SDL_BUTTON_LMASK) != 0,
		"Cross/A holds the effective left mouse button");
	Win32_Gamepad_Test_Set_Button(SDL_GAMEPAD_BUTTON_SOUTH, false);
	Check((Win32_Pointer_Buttons() & SDL_BUTTON_LMASK) == 0,
		"Cross/A release clears only the controller left-button source");

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
}


void Test_Right_Stick_Camera_Pan(void)
{
	Setup();
	Win32_Pointer_Move(250.0f, 200.0f);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTX, 32767);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTY, 32767);
	Service(SECOND);
	float x = 0.0f;
	float y = 0.0f;
	Win32_Pointer_Position(&x, &y);
	int panx = 0;
	int pany = 0;
	Check(x == 250.0f && y == 200.0f, "right stick leaves the visible cursor position unchanged");
	Check(Win32_Gamepad_Take_Camera_Pan(&panx, &pany) && panx > 0 && pany > 0,
		"right stick produces diagonal native camera-pan output");

	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTX, -32768);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTY, 32767);
	Service(2 * SECOND);
	Check(Win32_Gamepad_Take_Camera_Pan(&panx, &pany) && panx < 0 && pany > 0,
		"right-stick direction changes preserve independent x and y pan");

	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTX, 0);
	Win32_Gamepad_Test_Set_Axis(SDL_GAMEPAD_AXIS_RIGHTY, 0);
	Service(3 * SECOND);
	Check(!Win32_Gamepad_Take_Camera_Pan(&panx, &pany),
		"neutral right stick produces no further camera movement");
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
	Test_Stick_Dead_Zone_And_Bounds();
	Test_Speed_Boost();
	Test_Face_Buttons_And_Modifiers();
	Test_Start_And_Dpad();
	Test_Right_Stick_Camera_Pan();
	Test_Lifecycle_Release_And_Reconnect();
	std::printf("%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
