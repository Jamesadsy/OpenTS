/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <cmath>
#include <limits>
#include <utility>
#include <vector>


namespace UI_Controller_Focus
{

enum class Direction
{
	UP,
	DOWN,
	LEFT,
	RIGHT
};

struct TargetPosition
{
	float X;
	float Y;
};


inline int Next_Target_Index(std::vector<TargetPosition> const & targets,
	int current_index, Direction direction)
{
	if (targets.empty()) {
		return(-1);
	}

	bool const horizontal = direction == Direction::LEFT || direction == Direction::RIGHT;
	if (!horizontal || current_index < 0 || current_index >= (int)targets.size()) {
		bool const forward = direction == Direction::DOWN || direction == Direction::RIGHT;
		return(current_index < 0
			? (forward ? 0 : (int)targets.size() - 1)
			: (current_index + (forward ? 1 : (int)targets.size() - 1)) % (int)targets.size());
	}

	TargetPosition const current = targets[current_index];
	float best_score = std::numeric_limits<float>::max();
	int best_index = -1;
	for (int index = 0; index < (int)targets.size(); index++) {
		if (index == current_index) {
			continue;
		}
		TargetPosition const candidate = targets[index];
		float const primary = direction == Direction::RIGHT
			? candidate.X - current.X : current.X - candidate.X;
		if (primary <= 0.5f) {
			continue;
		}
		float const score = primary + 2.0f * std::abs(candidate.Y - current.Y);
		if (score < best_score) {
			best_score = score;
			best_index = index;
		}
	}

	if (best_index >= 0) {
		return(best_index);
	}
	return((current_index + (direction == Direction::RIGHT ? 1
		: (int)targets.size() - 1)) % (int)targets.size());
}

template<typename Activate>
bool Activate_Focused_Action(bool focus_mode, bool screen_owned_navigation,
	bool focused_action, bool disabled, Activate && activate)
{
	if (!focus_mode || screen_owned_navigation || !focused_action) {
		return(false);
	}
	if (!disabled) {
		std::forward<Activate>(activate)();
	}
	return(true);
}

}
