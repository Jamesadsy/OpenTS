/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "animationwaitpolicy.hh"
#include <cstdio>

int main(int, char **)
{
	struct Animation { bool Finished = false; bool Has_Finished() const { return(Finished); } } backdrop;
	struct Animations {
		Animation * Live;
		int ID(Animation * item) const { return(item == Live ? 0 : -1); }
		Animation * operator[](int) const { return(Live); }
	} animations{&backdrop};
	if (!MSEngine_Animation_Pending(animations, &backdrop)) return(1);
	backdrop.Finished = true;
	if (animations.ID(&backdrop) != 0 || MSEngine_Animation_Pending(animations, &backdrop)) return(2);
	animations.Live = nullptr;
	if (MSEngine_Animation_Pending(animations, &backdrop)) return(3);
	if (MSEngine_Animation_Pending(animations, (Animation *)nullptr)) return(4);
	std::puts("Persistent backdrop completion checks passed.");
	return(0);
}
