/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#pragma once

template<class AnimationList, class Animation>
bool MSEngine_Animation_Pending(AnimationList const & animations, Animation * animation)
{
	if (animation == nullptr) return(false);
	int const index = animations.ID(animation);
	return(index != -1 && !animations[index]->Has_Finished());
}
