/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "conquer.h"
#include "init.h"
#include "mainloopsession.h"

#include <cstdint>
#include <memory>

enum class MainGamePhase : std::uint8_t {
	Idle,
	InitGame,
	SelectGameBegin,
	SelectGame,
	PrepareGameplay,
	Gameplay,
	IngameModal,
	GameCleanup,
	ReturnFrontend,
	LaterBoundary,
	Complete,
	Fault,
};

enum class MainGameStepResult : std::uint8_t {
	Waiting,
	NativeChild,
	LaterBoundary,
	Complete,
	Fault,
};


// Native operations are injected so the owner can be contract-tested without starting the
// game, opening owner data, or entering a platform-specific startup path. Null operations
// use the accepted local SelectGame/MainLoop/IngameMenu sessions where there is a bounded
// session API. Scenario preparation and unknown native selection children are handed back
// to the caller and never run from Step.
struct MainGameSessionOps
{
	void * Context = nullptr;
	int (*InitGame)(void *, int, char **argv) = nullptr;
	bool (*BeginSelection)(void *, SelectGameSession &, bool fade) = nullptr;
	SelectGameStepResult (*StepSelection)(void *, SelectGameSession &) = nullptr;
	bool (*BeginGameplay)(void *, OpenTS_MainLoop::MainLoopSession &) = nullptr;
	OpenTS_MainLoop::CooperativeResult (*StepGameplay)(void *, OpenTS_MainLoop::MainLoopSession &) = nullptr;
	bool (*FinishGameplay)(void *, OpenTS_MainLoop::MainLoopSession &) = nullptr;
	bool (*ModalPending)(void *) = nullptr;
	bool (*BeginModal)(void *, IngameMenuSession &) = nullptr;
	IngameMenuStepResult (*StepModal)(void *, IngameMenuSession &) = nullptr;
	bool (*FinishModal)(void *, IngameMenuSession &) = nullptr;
	bool (*CleanupGameplay)(void *) = nullptr;
	bool (*ReturnFrontend)(void *, bool current_fade) = nullptr;
};


class MainGameSession
{
	public:
		MainGameSession(void);
		~MainGameSession(void);
		MainGameSession(MainGameSession const &) = delete;
		MainGameSession & operator=(MainGameSession const &) = delete;

		bool Begin(int argc, char ** argv, bool fade, MainGameSessionOps const & ops = MainGameSessionOps{}) noexcept;
		MainGameStepResult Step(void) noexcept;
		bool Complete_Selection_Native_Child(bool process, int selection, bool game_loaded,
			bool selection_from_menu = false) noexcept;
		bool Complete_Gameplay_Preparation(bool success) noexcept;
		bool Finish(void) noexcept;

	MainGamePhase Phase(void) const noexcept;
	SelectGameLaterBoundary Later_Boundary(void) const noexcept;
		bool Fade(void) const noexcept;
		bool Init_Game_Owned(void) const noexcept;
		bool Gameplay_Prepared(void) const noexcept;
		bool Cleanup_Owned(void) const noexcept;
		SelectGameSession * Selection(void) noexcept;
		OpenTS_MainLoop::MainLoopSession * Gameplay(void) noexcept;
		IngameMenuSession * Ingame_Menu(void) noexcept;

	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
};
