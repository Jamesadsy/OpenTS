/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <SDL3/SDL.h>

// Private deterministic hooks used only by the owner-data-free controller contract. They
// drive the production SDL3 mapping and motion code without requiring a physical device or
// opening a player-owned window.
void Win32_Gamepad_Test_Reset(void);
void Win32_Gamepad_Test_Set_Window_Size(float width, float height);
void Win32_Gamepad_Test_Set_Now(Uint64 now);
void Win32_Gamepad_Test_Set_Connected(bool connected);
void Win32_Gamepad_Test_Set_Axis(SDL_GamepadAxis axis, Sint16 value);
void Win32_Gamepad_Test_Set_Button(SDL_GamepadButton button, bool down);
void Win32_Gamepad_Test_Service(void);
bool Win32_Gamepad_Test_Take_Key_Event(int * virtualkey, bool * down);
bool Win32_Gamepad_Test_Take_Mouse_Event(Uint8 * button, bool * down,
	float * x, float * y);
