/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// The shell lets RmlUi see a modal keyboard message before the modal scope claims it.
// These adapters keep the ordering and propagation-to-consumption conversion together.

#pragma once

#include <RmlUi/Core/Context.h>


namespace UI_Modal_Input
{

inline bool Text_Consumed(Rml::Context & context, Rml::Character character, bool modal,
	bool developer_capture)
{
	if (developer_capture) {
		return(true);
	}

	bool const consumed = !context.ProcessTextInput(character);
	return(modal || consumed);
}


inline bool Key_Down_Consumed(Rml::Context & context, Rml::Input::KeyIdentifier identifier,
	int modifiers, bool modal, bool developer_capture)
{
	bool const consumed = !context.ProcessKeyDown(identifier, modifiers);
	return(modal || developer_capture || consumed);
}


inline bool Key_Up_Consumed(Rml::Context & context, Rml::Input::KeyIdentifier identifier,
	int modifiers, bool modal, bool developer_capture)
{
	bool const consumed = !context.ProcessKeyUp(identifier, modifiers);
	return(modal || developer_capture || consumed);
}

}
