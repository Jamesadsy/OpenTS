/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

#pragma once

#include <memory>

class ScenarioClass;

void Restate_Mission(ScenarioClass * scen);

class RestateMissionSession
{
	public:
		RestateMissionSession(void);
		~RestateMissionSession(void);
		bool Begin(ScenarioClass * scenario);
		int Step(void);
		void Finish(void);
	private:
		struct Impl;
		std::unique_ptr<Impl> State;
};
