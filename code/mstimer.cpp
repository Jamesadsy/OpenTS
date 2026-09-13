/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "mstimer.h"


/// <summary>
/// Fetches the current millisecond reading of the process-relative monotonic clock.
/// </summary>
int MillisecondSystemTimerClass::operator () (void) const
{
	return(static_cast<int>(System_Milliseconds()));
}


/// <summary>
/// Converts the timer into its current millisecond reading.
/// </summary>
MillisecondSystemTimerClass::operator int (void) const
{
	return(static_cast<int>(System_Milliseconds()));
}
