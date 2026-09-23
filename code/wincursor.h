/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// The mouse pointer, built from the game's own shapes. Windows composites its hardware
// cursor, while iOS receives an RGBA overlay from the same cached shape image.

#pragma once

#include "win.h"
#include "cursorpresentationpolicy.hh"

#include <cstdint>

class ShapeSet;


struct WinCursorOverlay
{
	unsigned char const * Pixels = nullptr;
	CursorPresentationSnapshot Presentation;
};


void Win_Cursor_Set(ShapeSet const * shape, int frame, int hotx, int hoty, bool apply);
void Win_Cursor_Set_Visible(bool visible);
bool Win_Cursor_Handle_Set_Cursor(void);
void Win_Cursor_Refresh(void);
bool Win_Cursor_Get_Overlay(WinCursorOverlay * overlay);
bool Win_Cursor_Is_Dirty(void);
void Win_Cursor_Acknowledge_Present(WinCursorOverlay const & submitted_overlay);
void Win_Cursor_Acknowledge_No_Overlay_Present(void);
void Win_Cursor_Shutdown(void);
