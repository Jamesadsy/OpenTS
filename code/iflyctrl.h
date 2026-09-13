/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <cstdint>



struct IFlyControl
{
	/*
	 * Landing altitude
	 */
	virtual std::int32_t Landing_Altitude(void) = 0;

	/*
	 * Lading direction
	 */
	virtual std::int32_t Landing_Direction(void) = 0;

	/*
	 * Loaded with cargo?
	 */
	virtual bool Is_Loaded(void) = 0;

	/*
	 * Does it strafe over the target rather than hover?
	 */
	virtual bool Is_Strafe(void) = 0;

	/*
	 * Is the aircraft locked into straight flight?
	 */
	virtual bool Is_Locked(void) = 0;
};

static_assert(sizeof(std::int32_t) == 4, "IFlyControl numeric values must remain 32-bit");
