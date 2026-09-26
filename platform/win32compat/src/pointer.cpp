/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "win32compat.h"

// The engine reads the pointer two ways at once. Mouse messages carry a position that the
// keyboard queue records, and the scroll handler, the gadgets, the tooltip timer and the
// placement cursor poll GetCursorPos and GetAsyncKeyState every frame. Both read the state
// kept here, so the message queue and the poll cannot disagree, and a host with no mouse
// answers the poll as well as a host with one.

static float _PointerX;
static float _PointerY;
static bool _PointerPositionInitialized;
static SDL_MouseButtonFlags _PointerButtons;
static SDL_MouseButtonFlags _HostButtons;
static SDL_MouseButtonFlags _ControllerButtons;
static bool _PointerStarted;
static bool _DirectTouch;
static bool _SuppressEdgeScroll;
static bool _ControllerOwnsPointer;
static bool _MenuFocus;


static SDL_MouseButtonFlags Button_Mask(Uint8 button)
{
	switch (button) {
		case SDL_BUTTON_LEFT: return(SDL_BUTTON_LMASK);
		case SDL_BUTTON_RIGHT: return(SDL_BUTTON_RMASK);
		case SDL_BUTTON_MIDDLE: return(SDL_BUTTON_MMASK);
		default: return(0);
	}
}


static void Set_Source_Button(SDL_MouseButtonFlags & source, Uint8 button, bool down)
{
	SDL_MouseButtonFlags const mask = Button_Mask(button);

	if (mask == 0) {
		return;
	}

	if (down) {
		source |= mask;
	} else {
		source &= ~mask;
	}

	_PointerButtons = _HostButtons | _ControllerButtons;
}


void Win32_Pointer_Move(float x, float y)
{
	_PointerX = x;
	_PointerY = y;
	_PointerPositionInitialized = true;
	_SuppressEdgeScroll = false;
}


bool Win32_Pointer_Initialize_Menu_Position(float width, float height)
{
	if (_PointerPositionInitialized || width <= 0.0f || height <= 0.0f) {
		return(false);
	}

	Win32_Pointer_Move((width - 1.0f) * 0.5f, (height - 1.0f) * 0.5f);
	return(true);
}


void Win32_Pointer_Move_Host(float x, float y)
{
	if (x != _PointerX || y != _PointerY) {
		Win32_Gamepad_Menu_Pointer_Moved();
	}
	Win32_Pointer_Set_Controller_Owner(false);
	Win32_Pointer_Move(x, y);
}


void Win32_Pointer_Move_Controller(float x, float y)
{
	_PointerX = x;
	_PointerY = y;
	_PointerPositionInitialized = true;
	_SuppressEdgeScroll = false;
	_ControllerOwnsPointer = true;
}


void Win32_Pointer_Set_Controller_Owner(bool controller_owns_pointer)
{
	_ControllerOwnsPointer = controller_owns_pointer;
}


bool Win32_Pointer_Is_Controller_Owner(void)
{
	return(_ControllerOwnsPointer);
}


void Win32_Pointer_Button(Uint8 button, bool down)
{
	Set_Source_Button(_HostButtons, button, down);
}


bool Win32_Pointer_Controller_Button(Uint8 button, bool down)
{
	SDL_MouseButtonFlags const before = _PointerButtons;
	Set_Source_Button(_ControllerButtons, button, down);
	return(before != _PointerButtons);
}


void Win32_Pointer_Position(float * x, float * y)
{
	if (x != NULL) *x = _PointerX;
	if (y != NULL) *y = _PointerY;
}


SDL_MouseButtonFlags Win32_Pointer_Buttons(void)
{
	return(_PointerButtons);
}


// Warping is what the tactical map's dragging scroll methods are built on: they pull the
// pointer back to the press point every frame so the map appears to travel under it. A host
// with no pointer of its own has nothing to pull, so it answers no and the game offers those
// methods to nobody.
bool Win32_Pointer_Can_Warp(void)
{
#ifdef OPENTS_IOS
	return(false);
#else
	return(true);
#endif
}


// The pointer's shape is where the engine says what a click would do, so a host that draws
// no pointer at all loses that. Nothing here decides what stands in for it; the answer is
// what lets the game decide.
bool Win32_Pointer_Is_Drawn(void)
{
	return(!_MenuFocus);
}


void Win32_Pointer_Set_Menu_Focus(bool focus)
{
	if (_MenuFocus == focus) {
		return;
	}
	_MenuFocus = focus;
	Win32_Input_Refresh_Cursor();
}


bool Win32_Pointer_Menu_Focus(void)
{
	return(_MenuFocus);
}


// A host reports motion only while its mouse is over a window it is delivering input to, and
// the engine polls a position whether it is or not, so the host is asked directly while it is
// not. The same call gives the pointer its opening position before the player has moved the
// mouse. The buttons are left to the events, which are the only place a press can arrive from.
void Win32_Pointer_Follow_Host_Mouse(void)
{
#ifndef OPENTS_IOS
	Win32Window * main = Win32_Lookup(Win32_Main_Window());

	if (main == NULL || main->Handle == NULL) {
		return;
	}

	bool const receiving = SDL_GetMouseFocus() == main->Handle && SDL_GetKeyboardFocus() == main->Handle;

	if (_PointerStarted && receiving) {
		return;
	}

	float x = 0.0f;
	float y = 0.0f;
	SDL_GetGlobalMouseState(&x, &y);

	int wx = 0;
	int wy = 0;
	SDL_GetWindowPosition(main->Handle, &wx, &wy);

	Win32_Pointer_Move_Host(x - (float)wx, y - (float)wy);

	if (!_PointerStarted) {
		_HostButtons = SDL_GetMouseState(NULL, NULL)
			& (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK | SDL_BUTTON_MMASK);
		_PointerButtons = _HostButtons | _ControllerButtons;
		_PointerStarted = true;
	}
#endif
}


bool Win32_Pointer_Is_Direct_Touch(void)
{
	return(_DirectTouch);
}


void Win32_Pointer_Set_Direct_Touch(bool direct)
{
	_DirectTouch = direct;
	if (direct) {
		Win32_Gamepad_Menu_Pointer_Moved();
		Win32_Pointer_Set_Controller_Owner(false);
	}
}


void Win32_Pointer_Suppress_Edge_Scroll(void)
{
	_SuppressEdgeScroll = true;
}


bool Win32_Pointer_Should_Suppress_Edge_Scroll(void)
{
	return(_SuppressEdgeScroll);
}
