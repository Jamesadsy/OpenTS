/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Synthetic, no-owner-data proof that drives the production touch recognizer.

#include <cstdio>

#include "touch_test.h"
#include "win32compat.h"

namespace
{

constexpr Uint64 MS = 1000000ULL;
int Failures = 0;

void Check(bool condition, char const * what)
{
	std::printf("%-66s %s\n", what, condition ? "ok" : "FAILED");
	if (!condition) Failures++;
}

SDL_Event Finger(Uint32 type, SDL_FingerID finger, float x, float y, Uint64 timestamp)
{
	SDL_Event event = {};
	event.type = type;
	event.tfinger.touchID = 1;
	event.tfinger.fingerID = finger;
	event.tfinger.x = x;
	event.tfinger.y = y;
	event.tfinger.timestamp = timestamp;
	return(event);
}

bool Send(SDL_Event const & event)
{
	return(Win32_Touch_Handle_Event(event));
}

bool Left_Down(void)
{
	return((Win32_Pointer_Buttons() & SDL_BUTTON_LMASK) != 0);
}

void Setup(void)
{
	Win32_Touch_Test_Reset();
	Win32_Touch_Test_Set_Window_Size(640.0f, 480.0f);
	Win32_Touch_Test_Set_Direct_Device(true);
	Win32_Touch_Test_Set_Now(0);
}

void Test_Tap(void)
{
	Setup();
	Check(Send(Finger(SDL_EVENT_FINGER_DOWN, 1, 0.25f, 0.25f, 0)),
		"production recognizer accepts a direct finger down");
	Check(Win32_Pointer_Is_Direct_Touch(),
		"direct touch owns the presentation pointer while the gesture is active");
	float x = 0.0f;
	float y = 0.0f;
	Win32_Pointer_Position(&x, &y);
	Check(x == 160.0f && y == 120.0f,
		"direct touch down moves the effective pointer to the finger");
	Check(!Left_Down(), "pending first finger emits no click");
	Check(Send(Finger(SDL_EVENT_FINGER_UP, 1, 0.25f, 0.25f, 10 * MS)),
		"production recognizer accepts a direct finger up");
	Check(!Left_Down(), "tap remains deferred until pointer settlement");
	Win32_Touch_Test_Set_Now(50 * MS);
	Win32_Touch_Service();
	Check(Left_Down(), "simple tap emits one primary-button press");
	Win32_Touch_Test_Set_Now(100 * MS);
	Win32_Touch_Service();
	Check(!Left_Down(), "simple tap releases its primary button");
	Win32_Pointer_Position(&x, &y);
	Check(x == 160.0f && y == 120.0f,
		"completed touch leaves the effective pointer at the last touch target");
	Check(!Win32_Pointer_Is_Direct_Touch(),
		"completed touch releases direct-touch ownership");
	Check(Win32_Pointer_Should_Suppress_Edge_Scroll(),
		"completed touch suppresses edge scrolling without moving the pointer");
}


void Test_Cancel(void)
{
	Setup();
	Send(Finger(SDL_EVENT_FINGER_DOWN, 1, 0.25f, 0.25f, 0));
	Win32_Touch_Cancel();
	Win32_Touch_Test_Set_Now(100 * MS);
	Win32_Touch_Service();
	Check(!Left_Down(), "cancel from pending emits no stray click");
}


void Test_Multi_Touch(void)
{
	Setup();
	Send(Finger(SDL_EVENT_FINGER_DOWN, 1, 0.20f, 0.20f, 0));
	Send(Finger(SDL_EVENT_FINGER_DOWN, 2, 0.40f, 0.20f, 1 * MS));
	Check(!Left_Down(), "second finger prevents a pending single-touch click");

	Send(Finger(SDL_EVENT_FINGER_MOTION, 2, 0.50f, 0.20f, 2 * MS));
	Send(Finger(SDL_EVENT_FINGER_MOTION, 2, 0.60f, 0.20f, 3 * MS));
	int scroll_x = 0;
	int scroll_y = 0;
	Check(Win32_Touch_Take_Scroll(&scroll_x, &scroll_y) && scroll_x != 0,
		"two-finger pan produces deterministic scroll output");
	Send(Finger(SDL_EVENT_FINGER_UP, 1, 0.20f, 0.20f, 4 * MS));
	Send(Finger(SDL_EVENT_FINGER_UP, 2, 0.60f, 0.20f, 5 * MS));
	Win32_Touch_Test_Set_Now(100 * MS);
	Win32_Touch_Service();
	Check(!Left_Down(), "multi-touch completion leaves no primary button held");
}


void Test_Drag_Cancel(void)
{
	Setup();
	Send(Finger(SDL_EVENT_FINGER_DOWN, 1, 0.20f, 0.20f, 0));
	Send(Finger(SDL_EVENT_FINGER_MOTION, 1, 0.40f, 0.20f, 1 * MS));
	float x = 0.0f;
	float y = 0.0f;
	Win32_Pointer_Position(&x, &y);
	Check(x == 256.0f && y == 96.0f,
		"one-finger motion tracks the actual effective pointer position");
	Check(Left_Down(), "one-finger drag holds the primary button");
	Win32_Touch_Cancel();
	Check(!Left_Down(), "cancelling a recognizer-owned drag releases its button");
}


void Test_Device_Qualification(void)
{
	Setup();
	Win32_Touch_Test_Set_Direct_Device(false);
	Check(!Send(Finger(SDL_EVENT_FINGER_DOWN, 1, 0.25f, 0.25f, 0)),
		"indirect touch devices remain on their own pointer path");
	Check(!Left_Down(), "indirect touch cannot inject a recognizer click");
}

}


int main(void)
{
	Test_Tap();
	Test_Cancel();
	Test_Multi_Touch();
	Test_Drag_Cancel();
	Test_Device_Qualification();
	std::printf("%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
