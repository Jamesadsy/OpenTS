/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#pragma once

struct VqaCooperativeState
{
	bool Sleeping = false;

	template<class Walk, class Pause, class Resume>
	bool Step(bool focus, bool pause_on_focus_loss, bool paused, bool & finished,
		Walk walk, Pause pause, Resume resume)
	{
		finished = false;
		if (Sleeping) {
			if (!focus) return(false);
			Sleeping = false;
			resume();
			paused = false;
		}
		if ((focus || !pause_on_focus_loss) && !paused) return(walk(finished));
		Sleeping = true;
		pause();
		return(false);
	}
};
