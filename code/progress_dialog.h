/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "point.h"

namespace ProgressDialog
{

#if defined(_WIN32)

void Begin(void);
void End(void);
bool Is_Active(void);
void Paint(void);
bool Bar_Center(Point2D & center);

#else

inline void Begin(void)
{
}

inline void End(void)
{
}

inline bool Is_Active(void)
{
	return(false);
}

inline void Paint(void)
{
}

inline bool Bar_Center(Point2D &)
{
	return(false);
}

#endif

} // namespace ProgressDialog
