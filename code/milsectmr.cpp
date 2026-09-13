/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "milsectmr.h"

#include "monotonic.h"


/// <summary>
/// Fetches process-relative monotonic elapsed time in milliseconds.
/// </summary>
MillisecondTimerClass::operator double () const
{
	return(static_cast<double>(Monotonic_Milliseconds()));
}
