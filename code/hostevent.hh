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


// Stable engine key identities used by native hosts.  Their values match the
// legacy keyboard queue so a host adapter does not need platform SDK key types.
enum OpenTSHostKey
{
	OPENTS_HOST_KEY_NONE = 0x00,
	OPENTS_HOST_KEY_LEFT_BUTTON = 0x01,
	OPENTS_HOST_KEY_RIGHT_BUTTON = 0x02,
	OPENTS_HOST_KEY_MIDDLE_BUTTON = 0x04,
	OPENTS_HOST_KEY_BACKSPACE = 0x08,
	OPENTS_HOST_KEY_TAB = 0x09,
	OPENTS_HOST_KEY_RETURN = 0x0D,
	OPENTS_HOST_KEY_SHIFT = 0x10,
	OPENTS_HOST_KEY_CONTROL = 0x11,
	OPENTS_HOST_KEY_ALT = 0x12,
	OPENTS_HOST_KEY_ESCAPE = 0x1B,
	OPENTS_HOST_KEY_SPACE = 0x20,
	OPENTS_HOST_KEY_PAGE_UP = 0x21,
	OPENTS_HOST_KEY_PAGE_DOWN = 0x22,
	OPENTS_HOST_KEY_END = 0x23,
	OPENTS_HOST_KEY_HOME = 0x24,
	OPENTS_HOST_KEY_LEFT = 0x25,
	OPENTS_HOST_KEY_UP = 0x26,
	OPENTS_HOST_KEY_RIGHT = 0x27,
	OPENTS_HOST_KEY_DOWN = 0x28,
	OPENTS_HOST_KEY_DELETE = 0x2E,
	OPENTS_HOST_KEY_F1 = 0x70,
	OPENTS_HOST_KEY_F2 = 0x71,
	OPENTS_HOST_KEY_F3 = 0x72,
	OPENTS_HOST_KEY_F4 = 0x73,
	OPENTS_HOST_KEY_F5 = 0x74,
	OPENTS_HOST_KEY_F6 = 0x75,
	OPENTS_HOST_KEY_F7 = 0x76,
	OPENTS_HOST_KEY_F8 = 0x77,
	OPENTS_HOST_KEY_F9 = 0x78,
	OPENTS_HOST_KEY_F10 = 0x79,
	OPENTS_HOST_KEY_F11 = 0x7A,
	OPENTS_HOST_KEY_F12 = 0x7B,
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
