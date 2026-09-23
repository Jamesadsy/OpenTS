/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once


template <typename MouseType, typename ShapeSet, typename Hotspot, typename ResetAnimation,
	typename CurrentFrame, typename SetCursor>
bool Mouse_Override_Shape_If_Changed(bool & startup, ShapeSet const * shapes, MouseType requested,
	MouseType & current, bool requested_small, bool & current_small, Hotspot const & hotspot,
	ResetAnimation && reset_animation, CurrentFrame && current_frame, SetCursor && set_cursor)
{
	if (!startup || (shapes != nullptr && (requested != current || requested_small != current_small))) {
		startup = true;
		reset_animation();
		set_cursor(hotspot, shapes, current_frame());
		current = requested;
		current_small = requested_small;
		return(true);
	}

	return(false);
}
