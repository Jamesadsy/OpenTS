/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <algorithm>


inline double Controller_ScrollRate_Scale(int scroll_rate, int scroll_rate_setting_count)
{
	if (scroll_rate_setting_count <= 0) {
		return(1.0);
	}

	int const last_setting = std::min(scroll_rate_setting_count - 1, 7);
	int const bounded_rate = std::clamp(scroll_rate, 0, last_setting);
	return((600.0 - 40.0 * (double)bounded_rate) / 480.0);
}


inline int Scale_Window_Scroll_Offset(int offset, double game_pixels_per_window_pixel, double speed_scale, double & remainder)
{
	double const scaled = (double)offset * game_pixels_per_window_pixel * speed_scale + remainder;
	int const distance = (int)scaled;
	remainder = scaled - distance;
	return(distance);
}


inline int Scale_Touch_Scroll_Offset(int offset, double game_pixels_per_window_pixel, double & remainder)
{
	return(Scale_Window_Scroll_Offset(offset, game_pixels_per_window_pixel, 1.0, remainder));
}


inline int Scale_Controller_Scroll_Offset(int offset, double game_pixels_per_window_pixel, int scroll_rate,
	int scroll_rate_setting_count, double & remainder)
{
	return(Scale_Window_Scroll_Offset(offset, game_pixels_per_window_pixel,
		Controller_ScrollRate_Scale(scroll_rate, scroll_rate_setting_count), remainder));
}
