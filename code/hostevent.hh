/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once


enum OpenTSHostEventType
{
	OPENTS_HOST_EVENT_NONE,
	OPENTS_HOST_EVENT_KEY,
	OPENTS_HOST_EVENT_MOUSE_MOVE,
	OPENTS_HOST_EVENT_MOUSE_BUTTON,
	OPENTS_HOST_EVENT_FOCUS,
	OPENTS_HOST_EVENT_QUIT,
};


struct OpenTSHostEvent
{
	OpenTSHostEventType Type = OPENTS_HOST_EVENT_NONE;
	unsigned short Key = 0;
	int X = 0;
	int Y = 0;
	bool Release = false;
	bool Shift = false;
	bool Control = false;
	bool Alt = false;
	bool Focused = false;
};
