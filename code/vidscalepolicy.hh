/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "video.h"

#include <cmath>

struct VideoPoint
{
	int X;
	int Y;
};

inline VideoPoint Window_Pixels_To_Game(VideoScaleInfo const & scale, VideoPoint point)
{
	if (scale.DestWidth > 0 && scale.DestHeight > 0) {
		point.X = (int)std::floor((point.X - scale.DestX) * (double)scale.GameWidth / scale.DestWidth);
		point.Y = (int)std::floor((point.Y - scale.DestY) * (double)scale.GameHeight / scale.DestHeight);
	}
	return(point);
}

inline VideoPoint Game_To_Drawable_Pixels(VideoScaleInfo const & scale, VideoPoint point)
{
	if (scale.GameWidth > 0 && scale.GameHeight > 0) {
		point.X = scale.DestX + (int)std::floor(point.X * (double)scale.DestWidth / scale.GameWidth);
		point.Y = scale.DestY + (int)std::floor(point.Y * (double)scale.DestHeight / scale.GameHeight);
	}
	return(point);
}
