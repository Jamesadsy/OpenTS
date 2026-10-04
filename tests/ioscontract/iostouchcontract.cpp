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
#include <cstdint>

#include "touch_test.h"
#include "movieholdpolicy.h"
#include "win32compat.h"
#include "vidscalepolicy.hh"
#include "keyboard.h"

bool GameInFocus = false;

void Windows_Message_Handler(void)
{
	MSG message = {};
	while (PeekMessage(&message, NULL, 0, 0, PM_NOREMOVE)) {
		if (!GetMessage(&message, NULL, 0, 0)) return;
		DispatchMessage(&message);
	}
}
void Clamp_To_Game(POINT &) {}

#if defined(__EMSCRIPTEN__)
extern "C" int OpenTS_Browser_Begin(void) { return(0); }
extern "C" void OpenTS_Browser_Main_Entry_Returned(void) {}
extern "C" void OpenTS_Browser_Telemetry_Increment(int) {}
extern "C" void OpenTS_Browser_Telemetry_Set(int, int) {}
extern "C" int OpenTS_Browser_Telemetry_Next_Event(void) { return(0); }
#endif

namespace
{

constexpr Uint64 MS = 1000000ULL;
int Failures = 0;

struct FirstTouchDispatchTrace
{
	int Sequence = 0;
	int ActivationBeforeClick = 0;
	int SetFocusBeforeClick = 0;
	int ClickSequence = 0;
	int FocusLossAfterClick = 0;
	bool FocusedAtClick = false;
	LPARAM ClickLParam = 0;
};

FirstTouchDispatchTrace FirstTouchTrace;
WWKeyboardClass NativeKeyboard;

LRESULT CALLBACK FirstTouchWindowProcedure(HWND, UINT message, WPARAM wparam, LPARAM lparam)
{
	++FirstTouchTrace.Sequence;
	if (message == WM_ACTIVATEAPP) {
		GameInFocus = wparam != 0;
		if (GameInFocus) {
			if (FirstTouchTrace.ClickSequence == 0) {
				FirstTouchTrace.ActivationBeforeClick = FirstTouchTrace.Sequence;
			}
		} else if (FirstTouchTrace.ClickSequence != 0) {
			FirstTouchTrace.FocusLossAfterClick = FirstTouchTrace.Sequence;
		}
	} else if (message == WM_SETFOCUS && GameInFocus
		&& FirstTouchTrace.ClickSequence == 0) {
		FirstTouchTrace.SetFocusBeforeClick = FirstTouchTrace.Sequence;
	} else if (message == WM_LBUTTONDOWN) {
		FirstTouchTrace.ClickSequence = FirstTouchTrace.Sequence;
		FirstTouchTrace.FocusedAtClick = GameInFocus;
		FirstTouchTrace.ClickLParam = lparam;
	}
	NativeKeyboard.Message_Handler(NULL, message, wparam, lparam);
	return(0);
}

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
	Check(Win32_Touch_Test_Button_Down_Count() == 1
		&& Win32_Touch_Test_Button_Up_Count() == 0,
		"one browser touch produces exactly one native menu-button down");
	Win32_Touch_Test_Set_Now(100 * MS);
	Win32_Touch_Service();
	Check(!Left_Down(), "simple tap releases its primary button");
	Check(Win32_Touch_Test_Button_Down_Count() == 1
		&& Win32_Touch_Test_Button_Up_Count() == 1,
		"the native menu-button up is delivered exactly once");
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


void Test_Movie_Hold(void)
{
	MovieSkipHold hold;
	Check(MovieSkipHold::HOLD_NS == 500 * MS, "TD/RA donor movie hold is 500 ms");
	hold.Press(MovieSkipHold::Source::Touch, 0);
	Check(!hold.Ready(100 * MS), "quick touch tap cannot skip");
	hold.Release(MovieSkipHold::Source::Touch);
	Check(!hold.Ready(700 * MS), "early touch release cancels the hold");
	hold.Press(MovieSkipHold::Source::Circle, 1000 * MS);
	Check(!hold.Ready(1499 * MS) && hold.Ready(1500 * MS)
		&& !hold.Ready(1600 * MS), "Circle skips once only after a deliberate hold");
	hold.Reset();
	hold.Press(MovieSkipHold::Source::Circle, 2000 * MS);
	hold.Release(MovieSkipHold::Source::Circle);
	Check(!hold.Ready(2600 * MS), "quick Circle press cannot skip another movie");
	hold.Press(MovieSkipHold::Source::Touch, 3000 * MS);
	Check(!hold.Ready(3499 * MS) && hold.Ready(3500 * MS),
		"touch hold skips at the same threshold after a movie reset");
	hold.Reset();
	Check(!hold.Ready(4000 * MS), "movie end clears all pending skip state");
}


void Test_First_Tap_Recovers_Focus_After_Late_Host_Loss(void)
{
	Setup();
	HWND const test_window = (HWND)(uintptr_t)1;
	Win32_Message_Test_Reset(test_window);
	Win32_Message_Test_Set_Procedure(FirstTouchWindowProcedure);
	FirstTouchTrace = {};
	NativeKeyboard.Clear();
	Win32_Window_Test_Set_Pixel_Density(3.0f);
	Win32_Touch_Test_Set_Window_Size(844.0f, 390.0f);

	// Native focus is still true until the queued loss is consumed, so the finger-down
	// activation check sees the stale state that allowed the physical click to overtake focus.
	GameInFocus = true;
	SDL_Event focus_lost = {};
	focus_lost.type = SDL_EVENT_WINDOW_FOCUS_LOST;
	Win32_Message_Test_Translate(focus_lost);
	Check(GameInFocus, "queued focus loss leaves native focus unchanged until message consumption");
	Win32_Message_Test_Translate(Finger(SDL_EVENT_FINGER_DOWN, 7, 0.5f, 0.5f, 0));
	Win32_Message_Test_Translate(Finger(SDL_EVENT_FINGER_UP, 7, 0.5f, 0.5f, 10 * MS));
	Win32_Touch_Test_Set_Now(50 * MS);
	Win32_Touch_Service();
	Check(Win32_Touch_Test_Button_Down_Count() == 1,
		"one intended first tap emits one selector click without a focus-only tap");

	Check(!Win32_Pointer_Is_Direct_Touch(),
		"finger-up retires gesture ownership before the settled click is emitted");
	Check(Win32_Touch_Click_Pending(),
		"settled synthetic click preserves touch origin after the finger gesture ends");

	// Match the reported ordering: a host focus-loss is consumed after the click has
	// already been posted. Dispatch restores focus for click routing, then the queued loss
	// leaves the game unfocused until the selector's pre-hit-test recovery runs.
	SDL_Event late_focus_lost = {};
	late_focus_lost.type = SDL_EVENT_WINDOW_FOCUS_LOST;
	Win32_Message_Test_Translate(late_focus_lost);

	MSG message = {};
	while (Win32_Message_Test_Pop(&message)) {
		DispatchMessage(&message);
	}
	Check(FirstTouchTrace.ActivationBeforeClick > 0
		&& FirstTouchTrace.SetFocusBeforeClick > FirstTouchTrace.ActivationBeforeClick
		&& FirstTouchTrace.ClickSequence > FirstTouchTrace.SetFocusBeforeClick,
		"native activation and set-focus are dispatched before the first touch click procedure");
	Check(FirstTouchTrace.FocusedAtClick,
		"first touch click reaches the native window procedure while focused");
	Check(FirstTouchTrace.FocusLossAfterClick > FirstTouchTrace.ClickSequence && !GameInFocus,
		"contract reproduces host focus loss consumed after the focused native click");
	Check(Win32_Touch_Click_Pending() && Win32_Restore_Focus_For_Touch_Click() && GameInFocus,
		"selector pre-hit-test recovery restores late focus while the first touch click is pending");

	// The live pump also consumes button-up before the next GraphicMenu Step.
	Win32_Touch_Test_Set_Now(100 * MS);
	Win32_Touch_Service();
	Win32_Message_Test_Translate(late_focus_lost);
	unsigned short const first_key = NativeKeyboard.Check();
	Check(!Left_Down() && !GameInFocus,
		"live ordering drains release and another host loss before selector evaluation");
	Check(Win32_Touch_Click_Pending(),
		"same first native click retains touch identity after its queued button-up");
	Check(Win32_Restore_Focus_For_Touch_Click() && GameInFocus,
		"selector can restore focus after button-up while the first click awaits consumption");
	Check(first_key == KN_LMOUSE,
		"actual native keyboard retains the first down ahead of its button-up");

	int const physical_x = (short)LOWORD(FirstTouchTrace.ClickLParam);
	int const physical_y = (short)HIWORD(FirstTouchTrace.ClickLParam);
	VideoScaleInfo const scale{640, 440, 2532, 1170, 415, 0, 1702, 1170,
		1702.0f / 640, 1170.0f / 440};
	VideoPoint const game_point = Window_Pixels_To_Game(scale, {physical_x, physical_y});
	Check(physical_x == 1266 && physical_y == 585,
		"DPR-3 touch logical coordinates become the expected 2532 by 1170 backing pixels");
	Check(game_point.X == 320 && game_point.Y == 220,
		"DPR-3 click maps through the scaled, letterboxed canvas to game-frame coordinates");

	struct RectFixture { int Result; int Left; int Top; int Right; int Bottom; };
	RectFixture const items[] = {
		{100, 100, 100, 540, 175},
		{101, 100, 185, 540, 265},
		{0, 100, 280, 540, 355},
	};
	int candidate_checks = 0;
	int active_rect_hits = 0;
	int native_result = -1;
	for (RectFixture const & item : items) {
		++candidate_checks;
		if (game_point.X >= item.Left && game_point.X < item.Right
			&& game_point.Y >= item.Top && game_point.Y < item.Bottom) {
			++active_rect_hits;
			native_result = item.Result;
			break;
		}
	}
	Check(candidate_checks == 2 && active_rect_hits == 1 && native_result == 101,
		"recovered first touch executes candidate checks and hits Firestorm's active rectangle");
	Check(NativeKeyboard.Get() == KN_LMOUSE,
		"native selector consumes the same first queued click");
	Check(!Win32_Touch_Click_Pending(),
		"selector consumption retires the first touch origin marker");
	Check(NativeKeyboard.Get() == ((unsigned short)KN_LMOUSE | (unsigned short)WWKEY_RLS_BIT),
		"native button-up remains queued in order after click consumption");
	Win32_Touch_Click_Begin();
	NativeKeyboard.Clear();
	Check(!Win32_Touch_Click_Pending(), "native input clear retires discarded touch identity");

	Win32_Touch_Test_Reset();
	Win32_Message_Test_Reset(NULL);
	Win32_Window_Test_Set_Pixel_Density(0.0f);
}

}


int main(int, char **)
{
	Test_Tap();
	Test_Cancel();
	Test_Multi_Touch();
	Test_Drag_Cancel();
	Test_Device_Qualification();
	Test_Movie_Hold();
#if defined(__EMSCRIPTEN__)
	Test_First_Tap_Recovers_Focus_After_Late_Host_Loss();
#endif
	std::printf("%s\n", Failures == 0 ? "All checks passed." : "There were failures.");
	return(Failures == 0 ? 0 : 1);
}
