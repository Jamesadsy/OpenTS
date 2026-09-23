/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "rect.h"


inline Rect Collapse_Tactical_Rect_For_Sidebar(Rect expanded, int side_width, bool sidebar_on_right)
{
	return(Rect(sidebar_on_right ? expanded.X : expanded.X - side_width, expanded.Y,
		expanded.Width + side_width, expanded.Height));
}
