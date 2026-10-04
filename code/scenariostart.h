/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#pragma once

#include "campaign.hh"
#include <memory>

class ScenarioStartSession
{
	public:
		ScenarioStartSession(void);
		~ScenarioStartSession(void);
		bool Begin(char const * name, bool briefing, CampaignType campaign);
		int Step(void);
	private:
		struct Impl;
		std::unique_ptr<Impl> State;
};
