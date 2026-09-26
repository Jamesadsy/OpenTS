/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <utility>


namespace UI_Controller_Back
{

template<typename Activate>
bool Activate_Declared_Target(bool declared, bool has_click_action, bool visible,
	bool disabled, Activate && activate)
{
	if (!declared || !has_click_action || !visible) {
		return(false);
	}
	if (!disabled) {
		std::forward<Activate>(activate)();
	}
	return(true);
}

}
