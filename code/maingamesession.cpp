/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "maingamesession.h"

#include "globals.h"
#include "mainloop.h"
#include "special.hh"


struct MainGameSession::Impl
{
	MainGamePhase Phase = MainGamePhase::Idle;
	MainGameSessionOps Ops;
	SelectGameSession Selection;
	OpenTS_MainLoop::MainLoopSession MainLoop;
	IngameMenuSession Ingame;
	int Argc = 0;
	char ** Argv = nullptr;
	bool Fade = true;
	bool InitOwned = false;
	bool SelectionBegun = false;
	bool PreparationOwned = false;
	bool GameplayPrepared = false;
	bool MainLoopBegun = false;
	bool IngameBegun = false;
	bool CleanupOwned = false;
	bool GameWasCompleted = false;
	bool StepActive = false;
	bool Finished = false;
};


MainGameSession::MainGameSession(void) : m_Impl(std::make_unique<Impl>())
{
}


MainGameSession::~MainGameSession(void)
{
	if (m_Impl != nullptr && !m_Impl->Finished) Finish();
}


bool MainGameSession::Begin(int argc, char ** argv, bool fade, MainGameSessionOps const & ops) noexcept
{
	if (m_Impl->StepActive) return(false);
	if (m_Impl->MainLoopBegun && m_Impl->Ops.FinishGameplay != nullptr) {
		m_Impl->Ops.FinishGameplay(m_Impl->Ops.Context, m_Impl->MainLoop);
	}
	if (m_Impl->IngameBegun && m_Impl->Ops.FinishModal != nullptr) {
		m_Impl->Ops.FinishModal(m_Impl->Ops.Context, m_Impl->Ingame);
	}

	m_Impl->Phase = MainGamePhase::InitGame;
	m_Impl->Ops = ops;
	m_Impl->Argc = argc;
	m_Impl->Argv = argv;
	m_Impl->Fade = fade;
	m_Impl->InitOwned = false;
	m_Impl->SelectionBegun = false;
	m_Impl->PreparationOwned = false;
	m_Impl->GameplayPrepared = false;
	m_Impl->MainLoopBegun = false;
	m_Impl->IngameBegun = false;
	m_Impl->CleanupOwned = false;
	m_Impl->GameWasCompleted = false;
	m_Impl->Finished = false;
	m_Impl->MainLoop = OpenTS_MainLoop::MainLoopSession{};
	m_Impl->Ingame.Begin(false);

	if (m_Impl->Ops.InitGame != nullptr) {
		if (m_Impl->Ops.InitGame(m_Impl->Ops.Context, argc, argv) != 0) {
			m_Impl->Phase = MainGamePhase::Complete;
			m_Impl->InitOwned = true;
			return(false);
		}
	} else if (Init_Game(argc, argv) != 0) {
		m_Impl->Phase = MainGamePhase::Complete;
		m_Impl->InitOwned = true;
		return(false);
	}
	m_Impl->InitOwned = true;
	m_Impl->Phase = MainGamePhase::SelectGameBegin;
	return(true);
}


MainGameStepResult MainGameSession::Step(void) noexcept
{
	using Result = MainGameStepResult;
	if (m_Impl->Finished) return(m_Impl->Phase == MainGamePhase::Complete ? Result::Complete : Result::Fault);
	if (m_Impl->StepActive) {
		m_Impl->Phase = MainGamePhase::Fault;
		return(Result::Fault);
	}
	m_Impl->StepActive = true;
	struct StepGuard { bool & Active; ~StepGuard() { Active = false; } } guard{m_Impl->StepActive};

	switch (m_Impl->Phase) {
		case MainGamePhase::Idle:
		case MainGamePhase::InitGame:
			m_Impl->Phase = MainGamePhase::Fault;
			return(Result::Fault);

		case MainGamePhase::SelectGameBegin: {
			bool begun = false;
			if (m_Impl->Ops.BeginSelection != nullptr) {
				begun = m_Impl->Ops.BeginSelection(m_Impl->Ops.Context, m_Impl->Selection, m_Impl->Fade);
			} else {
				m_Impl->Selection.Begin(m_Impl->Fade);
				begun = true;
			}
			if (!begun) {
				m_Impl->Phase = MainGamePhase::Fault;
				return(Result::Fault);
			}
			m_Impl->SelectionBegun = true;
			m_Impl->Phase = MainGamePhase::SelectGame;
			return(Result::Waiting);
		}

		case MainGamePhase::SelectGame: {
			SelectGameStepResult const selection = m_Impl->Ops.StepSelection != nullptr
				? m_Impl->Ops.StepSelection(m_Impl->Ops.Context, m_Impl->Selection)
				: m_Impl->Selection.Step();
				switch (selection) {
				case SelectGameStepResult::Waiting:
					return(Result::Waiting);
				case SelectGameStepResult::NativeChild:
					if (m_Impl->Selection.Phase == SelectGamePhase::ScenarioPrepare) {
						m_Impl->Phase = MainGamePhase::PrepareGameplay;
					}
					return(Result::NativeChild);
				case SelectGameStepResult::LaterBoundary:
					m_Impl->Phase = MainGamePhase::LaterBoundary;
					return(Result::LaterBoundary);
				case SelectGameStepResult::ReadyForGameplay:
					m_Impl->Phase = MainGamePhase::PrepareGameplay;
					return(Result::NativeChild);
				case SelectGameStepResult::Exited:
					m_Impl->GameWasCompleted = false;
					m_Impl->Phase = MainGamePhase::ReturnFrontend;
					return(Result::Waiting);
				case SelectGameStepResult::Fault:
					m_Impl->Phase = MainGamePhase::Fault;
					return(Result::Fault);
			}
			m_Impl->Phase = MainGamePhase::Fault;
			return(Result::Fault);
		}

		case MainGamePhase::PrepareGameplay:
			// Scenario preparation retains native ownership. In particular, this is where
			// VQA/movie, renderer, and startup work remain outside the cooperative Step path.
			return(Result::NativeChild);

		case MainGamePhase::Gameplay: {
			bool modal_pending = false;
			if (m_Impl->Ops.ModalPending != nullptr) {
				modal_pending = m_Impl->Ops.ModalPending(m_Impl->Ops.Context);
			} else {
				modal_pending = (SpecialDialog != SDLG_NONE);
			}
			if (modal_pending) {
				bool begun = false;
				if (m_Impl->Ops.BeginModal != nullptr) {
					begun = m_Impl->Ops.BeginModal(m_Impl->Ops.Context, m_Impl->Ingame);
				} else {
					m_Impl->Ingame.Begin(false);
					begun = true;
				}
				if (!begun) {
					m_Impl->Phase = MainGamePhase::Fault;
					return(Result::Fault);
				}
				m_Impl->IngameBegun = true;
				m_Impl->Phase = MainGamePhase::IngameModal;
				return(Result::Waiting);
			}

			if (!m_Impl->MainLoopBegun) {
				bool begun = false;
				if (m_Impl->Ops.BeginGameplay != nullptr) {
					begun = m_Impl->Ops.BeginGameplay(m_Impl->Ops.Context, m_Impl->MainLoop);
				} else {
					begun = Main_Loop_Session_Begin(m_Impl->MainLoop);
				}
				if (!begun) {
					m_Impl->Phase = MainGamePhase::Fault;
					return(Result::Fault);
				}
				m_Impl->MainLoopBegun = true;
			}

			OpenTS_MainLoop::CooperativeResult const frame = m_Impl->Ops.StepGameplay != nullptr
				? m_Impl->Ops.StepGameplay(m_Impl->Ops.Context, m_Impl->MainLoop)
				: Main_Loop_Session_Step(m_Impl->MainLoop);
			if (frame == OpenTS_MainLoop::CooperativeResult::Active
				|| frame == OpenTS_MainLoop::CooperativeResult::Waiting) {
				return(Result::Waiting);
			}
			if (frame == OpenTS_MainLoop::CooperativeResult::Fault) {
				m_Impl->Phase = MainGamePhase::Fault;
				return(Result::Fault);
			}

			bool const finished = m_Impl->Ops.FinishGameplay != nullptr
				? m_Impl->Ops.FinishGameplay(m_Impl->Ops.Context, m_Impl->MainLoop)
				: Main_Loop_Session_Finish(m_Impl->MainLoop);
			m_Impl->MainLoopBegun = false;
			if (!finished) {
				m_Impl->Phase = MainGamePhase::Fault;
				return(Result::Fault);
			}
			m_Impl->Phase = MainGamePhase::GameCleanup;
			return(Result::Waiting);
		}

		case MainGamePhase::IngameModal: {
			IngameMenuStepResult const modal = m_Impl->Ops.StepModal != nullptr
				? m_Impl->Ops.StepModal(m_Impl->Ops.Context, m_Impl->Ingame)
				: m_Impl->Ingame.Step();
			if (modal == IngameMenuStepResult::Active || modal == IngameMenuStepResult::Waiting) {
				return(Result::Waiting);
			}
			if (modal == IngameMenuStepResult::Fault) {
				m_Impl->Phase = MainGamePhase::Fault;
				return(Result::Fault);
			}
			bool const finished = m_Impl->Ops.FinishModal != nullptr
				? m_Impl->Ops.FinishModal(m_Impl->Ops.Context, m_Impl->Ingame)
				: m_Impl->Ingame.Finish();
			m_Impl->IngameBegun = false;
			if (!finished) {
				m_Impl->Phase = MainGamePhase::Fault;
				return(Result::Fault);
			}
			m_Impl->Phase = MainGamePhase::Gameplay;
			return(Result::Waiting);
		}

		case MainGamePhase::GameCleanup:
			if (!m_Impl->CleanupOwned) {
				if (m_Impl->Ops.CleanupGameplay != nullptr
					&& !m_Impl->Ops.CleanupGameplay(m_Impl->Ops.Context)) {
					m_Impl->Phase = MainGamePhase::Fault;
					return(Result::Fault);
				}
				m_Impl->CleanupOwned = true;
				m_Impl->GameWasCompleted = true;
				m_Impl->Fade = true;
			}
			m_Impl->Phase = MainGamePhase::ReturnFrontend;
			return(Result::Waiting);

		case MainGamePhase::ReturnFrontend: {
			bool const again = m_Impl->Ops.ReturnFrontend != nullptr
				? m_Impl->Ops.ReturnFrontend(m_Impl->Ops.Context, m_Impl->Fade)
				: m_Impl->GameWasCompleted;
			if (!again) {
				m_Impl->Phase = MainGamePhase::Complete;
				return(Result::Complete);
			}
			m_Impl->Selection.Reset();
			m_Impl->SelectionBegun = false;
			m_Impl->PreparationOwned = false;
			m_Impl->GameplayPrepared = false;
			m_Impl->CleanupOwned = false;
			m_Impl->GameWasCompleted = false;
			m_Impl->MainLoop = OpenTS_MainLoop::MainLoopSession{};
			m_Impl->Phase = MainGamePhase::SelectGameBegin;
			return(Result::Waiting);
		}

		case MainGamePhase::Complete:
			return(Result::Complete);

		case MainGamePhase::LaterBoundary:
			return(Result::LaterBoundary);

		case MainGamePhase::Fault:
			return(Result::Fault);
	}

	m_Impl->Phase = MainGamePhase::Fault;
	return(Result::Fault);
}


bool MainGameSession::Complete_Selection_Native_Child(
	bool process, int selection, bool game_loaded, bool selection_from_menu) noexcept
{
	if (m_Impl->Phase != MainGamePhase::SelectGame
		|| m_Impl->Selection.Phase != SelectGamePhase::NativeChild) return(false);
	return(m_Impl->Selection.Complete_Native_Child(process, selection, game_loaded, selection_from_menu));
}


bool MainGameSession::Complete_Gameplay_Preparation(bool success) noexcept
{
	if (m_Impl->Phase != MainGamePhase::PrepareGameplay || m_Impl->PreparationOwned) return(false);
	m_Impl->PreparationOwned = true;
	if (!success) {
		m_Impl->Selection.Reset();
		m_Impl->SelectionBegun = false;
		m_Impl->Phase = MainGamePhase::SelectGameBegin;
		return(true);
	}
	m_Impl->GameplayPrepared = true;
	m_Impl->Phase = MainGamePhase::Gameplay;
	return(true);
}


bool MainGameSession::Finish(void) noexcept
{
	if (m_Impl->Finished) return(true);
	bool success = true;
	if (m_Impl->MainLoopBegun) {
		if (m_Impl->Ops.FinishGameplay != nullptr) {
			success = m_Impl->Ops.FinishGameplay(m_Impl->Ops.Context, m_Impl->MainLoop) && success;
		} else {
			success = Main_Loop_Session_Finish(m_Impl->MainLoop) && success;
		}
		m_Impl->MainLoopBegun = false;
	}
	if (m_Impl->IngameBegun) {
		if (m_Impl->Ops.FinishModal != nullptr) {
			success = m_Impl->Ops.FinishModal(m_Impl->Ops.Context, m_Impl->Ingame) && success;
		} else {
			success = m_Impl->Ingame.Finish() && success;
		}
		m_Impl->IngameBegun = false;
	}
	m_Impl->Finished = true;
	return(success);
}


MainGamePhase MainGameSession::Phase(void) const noexcept { return(m_Impl->Phase); }
SelectGameLaterBoundary MainGameSession::Later_Boundary(void) const noexcept
{
	return(m_Impl->Selection.Later_Boundary());
}
bool MainGameSession::Fade(void) const noexcept { return(m_Impl->Fade); }
bool MainGameSession::Init_Game_Owned(void) const noexcept { return(m_Impl->InitOwned); }
bool MainGameSession::Gameplay_Prepared(void) const noexcept { return(m_Impl->GameplayPrepared); }
bool MainGameSession::Cleanup_Owned(void) const noexcept { return(m_Impl->CleanupOwned); }
SelectGameSession * MainGameSession::Selection(void) noexcept { return(&m_Impl->Selection); }
OpenTS_MainLoop::MainLoopSession * MainGameSession::Gameplay(void) noexcept { return(&m_Impl->MainLoop); }
IngameMenuSession * MainGameSession::Ingame_Menu(void) noexcept { return(&m_Impl->Ingame); }
