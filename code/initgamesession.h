/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#pragma once

#include <cstdint>

enum class InitGameStepResult : std::uint8_t { Waiting, Complete, Fault };

struct InitGameSessionOps
{
	void * Context = nullptr;
	int (*Prepare)(void *) = nullptr;
	InitGameStepResult (*BeginMovie)(void *, unsigned) = nullptr;
	InitGameStepResult (*StepMovie)(void *) = nullptr;
	void (*FinishMovie)(void *) = nullptr;
	int (*Complete)(void *) = nullptr;
};

class InitGameSession
{
	public:
		bool Begin(InitGameSessionOps const & ops) noexcept;
		InitGameStepResult Step(void) noexcept;
		void Finish(void) noexcept;

	private:
		enum class Phase : std::uint8_t { Idle, Prepare, BeginMovie, Movie, CompleteInit, Complete, Fault };
		Phase State = Phase::Idle;
		InitGameSessionOps Ops;
		unsigned MovieIndex = 0;
		bool MovieOwned = false;
};
