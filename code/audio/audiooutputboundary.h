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

// A renderer acknowledgement says the mixer stopped reading a source. The
// frames already handed to the output backend still need to pass its bounded
// queue before that source slot is safe to reuse.
inline unsigned Audio_Output_Drain_Target(unsigned renderedFrames, unsigned deviceLatencyFrames, unsigned periodFrames)
{
	return(renderedFrames + deviceLatencyFrames + periodFrames);
}


inline bool Audio_Output_Drain_Reached(unsigned renderedFrames, unsigned targetFrame)
{
	// Unsigned subtraction deliberately handles the monotonic frame counter's
	// wraparound, provided a single drain is less than 2^31 frames.
	return((renderedFrames - targetFrame) < 0x80000000u);
}
