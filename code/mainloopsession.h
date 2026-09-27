/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <cstdint>

namespace OpenTS_MainLoop
{

enum class CooperativeResult : int
{
	Active = 0,
	Waiting = 1,
	Complete = 2,
	Fault = 3
};

enum class MainLoopPhase : std::uint8_t
{
	Idle,
	FrameLogic,
	Pace,
	PostFrame,
	Complete,
	Fault
};

enum class SyncDelayPhase : std::uint8_t
{
	Idle,
	LocalWait,
	NetworkWait,
	Complete,
	Fault
};

struct SyncDelaySession
{
	SyncDelayPhase Phase = SyncDelayPhase::Idle;
	bool NetworkWait = false;
	bool BeginOwned = false;
	bool StepActive = false;
	bool FinishOwned = false;
	bool NativeInnerYield = false;
	bool NativeTailYield = false;
	bool NativeTimerCheckInService = false;
	int FaultCode = 0;
};

struct SyncDelaySessionOps
{
	void * Context = nullptr;
	bool (*IsNetworkWait)(void *) = nullptr;
	void (*AccumulateSpareTicks)(void *) = nullptr;
	void (*ServiceOnce)(void *, SyncDelaySession &) = nullptr;
	int (*TimeRemaining)(void *, bool) = nullptr;
	void (*OnComplete)(void *) = nullptr;
};

struct MainLoopSession
{
	MainLoopPhase Phase = MainLoopPhase::Idle;
	bool BeginOwned = false;
	bool LoopStarted = false;
	bool StepActive = false;
	bool Done = false;
	bool Result = false;
	bool PostFrameOwned = false;
	bool FinishOwned = false;
	int FaultCode = 0;
	SyncDelaySession Pacing;
};

struct MainLoopSessionOps
{
	void * Context = nullptr;
	bool (*GameActive)(void *) = nullptr;
	void (*EnterMainLoop)(void *) = nullptr;
	bool (*FocusReady)(void *) = nullptr;
	bool (*RunFrameLogic)(void *) = nullptr;
	SyncDelaySessionOps Pacing;
	void (*PostFrame)(void *, bool) = nullptr;
	bool (*GameEnded)(void *) = nullptr;
};

bool MainLoopSession_Begin(MainLoopSession & session) noexcept;
CooperativeResult MainLoopSession_Step(MainLoopSession & session, MainLoopSessionOps const & ops) noexcept;
bool MainLoopSession_Finish(MainLoopSession & session) noexcept;

bool SyncDelaySession_Begin(SyncDelaySession & session, SyncDelaySessionOps const & ops) noexcept;
CooperativeResult SyncDelaySession_Step(SyncDelaySession & session, SyncDelaySessionOps const & ops) noexcept;
bool SyncDelaySession_Finish(SyncDelaySession & session) noexcept;

} // namespace OpenTS_MainLoop
