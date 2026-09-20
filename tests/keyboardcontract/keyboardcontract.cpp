/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "keyboard.h"

#include <cstdio>
#include <initializer_list>


void Windows_Message_Handler(void)
{
}


void Clamp_To_Game(POINT & point)
{
	(void)point;
}


static int _Failures;


static void Check(bool condition, char const * description)
{
	if (!condition) {
		std::fprintf(stderr, "keyboard-contract: %s\n", description);
		_Failures++;
	}
}


static unsigned short Take(WWKeyboardClass & keyboard, char expected)
{
	unsigned short const key = keyboard.Get();
	char const description[] = "literal character is queued";
	Check((key & 0xFF) == (unsigned char)expected, description);
	Check((key & WWKEY_TEXT_BIT) != 0, "literal character carries the text marker");
	Check(keyboard.To_ASCII(key) == (unsigned char)expected, "literal character converts once");
	return(key);
}


int main(void)
{
	Check(WWKEY_TEXT_BIT == 0x2000, "text marker occupies the dedicated 0x2000 slot");
	Check(WWKEY_UNK_BIT == 0x4000, "legacy unknown/button-release marker remains unchanged");

	WWKeyboardClass keyboard;

	for (char const character : {'A', 'B', 'C', '1', '2', '3'}) {
		Check(keyboard.Message_Handler(NULL, WM_CHAR, (WPARAM)(unsigned char)character, 0),
			"WM_CHAR is consumed by the legacy queue");
	}

	for (char const character : {'A', 'B', 'C', '1', '2', '3'}) {
		Take(keyboard, character);
	}
	Check(keyboard.Check() == 0, "ABC123 produces no duplicate queue entries");

	Check(keyboard.Message_Handler(NULL, WM_CHAR, (WPARAM)'?', 0),
		"another printable WM_CHAR is accepted");
	Take(keyboard, '?');

	Check(keyboard.Message_Handler(NULL, WM_CHAR, (WPARAM)0x01, 0),
		"non-printable WM_CHAR is consumed without entering the literal queue");
	Check(keyboard.Check() == 0, "non-printable text does not become legacy text");

	Check(keyboard.Message_Handler(NULL, WM_KEYDOWN, VK_RETURN, 0),
		"Return keeps the ordinary key path");
	unsigned short const enter = keyboard.Get();
	Check((enter & 0xFF) == VK_RETURN && (enter & WWKEY_TEXT_BIT) == 0,
		"Return is not marked as text");

	Check(keyboard.Message_Handler(NULL, WM_KEYDOWN, VK_BACK, 0),
		"Backspace keeps the ordinary key path");
	unsigned short const backspace = keyboard.Get();
	Check((backspace & 0xFF) == VK_BACK && (backspace & WWKEY_TEXT_BIT) == 0,
		"Backspace is not marked as text");

	Check(keyboard.Message_Handler(NULL, WM_KEYDOWN, VK_LEFT, 0),
		"Left keeps the ordinary key path");
	unsigned short const left = keyboard.Get();
	Check((left & 0xFF) == VK_LEFT && (left & WWKEY_TEXT_BIT) == 0,
		"Left is not marked as text");

	Check(keyboard.Message_Handler(NULL, WM_KEYDOWN, VK_RIGHT, 0),
		"Right keeps the ordinary key path");
	unsigned short const right = keyboard.Get();
	Check((right & 0xFF) == VK_RIGHT && (right & WWKEY_TEXT_BIT) == 0,
		"Right is not marked as text");

	return(_Failures == 0 ? 0 : 1);
}
