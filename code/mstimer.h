/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "monotonic.h"

class MillisecondSystemTimerClass
{
	public:
		// The monotonic service starts in this process; save/load must rebase it.
		static constexpr bool Reading_Survives_A_Save = false;

		int operator () (void) const;
		operator int (void) const;
};
