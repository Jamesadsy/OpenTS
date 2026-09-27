/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "mainloopsession.h"

namespace
{

constexpr int FaultInvalidPhase = 1;
constexpr int FaultInvalidOperations = 2;
constexpr int FaultReentrantStep = 3;
constexpr int FaultInvalidChild = 4;

struct SessionStepGuard
{
	explicit SessionStepGuard(bool & active) noexcept : Active(active)
	{
		Active = true;
	}

	~SessionStepGuard() noexcept
	{
		Active = false;
	}

	bool & Active;
};

bool HasValidSyncDelayOps(OpenTS_MainLoop::SyncDelaySessionOps const & ops) noexcept
{
	return ops.IsNetworkWait != nullptr
		&& ops.AccumulateSpareTicks != nullptr
		&& ops.ServiceOnce != nullptr
		&& ops.TimeRemaining != nullptr
		&& ops.OnComplete != nullptr;
}

bool HasValidMainLoopOps(OpenTS_MainLoop::MainLoopSessionOps const & ops) noexcept
{
	return ops.GameActive != nullptr
		&& ops.EnterMainLoop != nullptr
		&& ops.FocusReady != nullptr
		&& ops.RunFrameLogic != nullptr
		&& HasValidSyncDelayOps(ops.Pacing)
		&& ops.PostFrame != nullptr
		&& ops.GameEnded != nullptr;
}

OpenTS_MainLoop::CooperativeResult SetSyncDelayFault(
	OpenTS_MainLoop::SyncDelaySession & session, int code) noexcept
{
	session.Phase = OpenTS_MainLoop::SyncDelayPhase::Fault;
	session.FaultCode = code;
	return OpenTS_MainLoop::CooperativeResult::Fault;
}

OpenTS_MainLoop::CooperativeResult SetMainLoopFault(
	OpenTS_MainLoop::MainLoopSession & session, int code) noexcept
{
	session.Phase = OpenTS_MainLoop::MainLoopPhase::Fault;
	session.FaultCode = code;
	return OpenTS_MainLoop::CooperativeResult::Fault;
}

OpenTS_MainLoop::CooperativeResult CompletePostFrame(
	OpenTS_MainLoop::MainLoopSession & session,
	OpenTS_MainLoop::MainLoopSessionOps const & ops) noexcept
{
	if (session.PostFrameOwned) {
		return SetMainLoopFault(session, FaultInvalidPhase);
	}

	session.PostFrameOwned = true;
	ops.PostFrame(ops.Context, session.Done);
	session.Result = ops.GameEnded(ops.Context);
	session.Phase = OpenTS_MainLoop::MainLoopPhase::Complete;
	return OpenTS_MainLoop::CooperativeResult::Complete;
}

} // namespace

namespace OpenTS_MainLoop
{

bool MainLoopSession_Begin(MainLoopSession & session) noexcept
{
	if (session.BeginOwned || session.FinishOwned || session.Phase != MainLoopPhase::Idle) {
		return false;
	}

	session.BeginOwned = true;
	session.Phase = MainLoopPhase::FrameLogic;
	return true;
}


CooperativeResult MainLoopSession_Step(MainLoopSession & session, MainLoopSessionOps const & ops) noexcept
{
	if (!session.BeginOwned) {
		return CooperativeResult::Fault;
	}
	if (session.StepActive) {
		return SetMainLoopFault(session, FaultReentrantStep);
	}
	if (session.Phase == MainLoopPhase::Complete) {
		return CooperativeResult::Complete;
	}
	if (session.Phase == MainLoopPhase::Fault) {
		return CooperativeResult::Fault;
	}
	if (!HasValidMainLoopOps(ops)) {
		return SetMainLoopFault(session, FaultInvalidOperations);
	}

	SessionStepGuard active(session.StepActive);
	switch (session.Phase) {
		case MainLoopPhase::FrameLogic:
			if (!session.LoopStarted) {
				if (!ops.GameActive(ops.Context)) {
					session.Done = true;
					session.Result = true;
					session.Phase = MainLoopPhase::Complete;
					return CooperativeResult::Complete;
				}
				ops.EnterMainLoop(ops.Context);
				session.LoopStarted = true;
			}

			if (!ops.FocusReady(ops.Context)) {
				return CooperativeResult::Waiting;
			}

			session.Done = ops.RunFrameLogic(ops.Context);
			if (session.Done) {
				session.Phase = MainLoopPhase::PostFrame;
				return CompletePostFrame(session, ops);
			}

			if (!SyncDelaySession_Begin(session.Pacing, ops.Pacing)) {
				return SetMainLoopFault(session, FaultInvalidChild);
			}
			session.Phase = MainLoopPhase::Pace;
			[[fallthrough]];

		case MainLoopPhase::Pace: {
			CooperativeResult const pacing_result = SyncDelaySession_Step(session.Pacing, ops.Pacing);
			if (pacing_result == CooperativeResult::Waiting) {
				return CooperativeResult::Waiting;
			}
			if (pacing_result != CooperativeResult::Complete) {
				return SetMainLoopFault(session, FaultInvalidChild);
			}
			session.Phase = MainLoopPhase::PostFrame;
			return CompletePostFrame(session, ops);
		}

		case MainLoopPhase::PostFrame:
			return CompletePostFrame(session, ops);

		case MainLoopPhase::Idle:
			return SetMainLoopFault(session, FaultInvalidPhase);

		case MainLoopPhase::Complete:
			return CooperativeResult::Complete;

		case MainLoopPhase::Fault:
			return CooperativeResult::Fault;
	}

	return SetMainLoopFault(session, FaultInvalidPhase);
}


bool MainLoopSession_Finish(MainLoopSession & session) noexcept
{
	if (session.FinishOwned) {
		return true;
	}
	if (!session.BeginOwned || session.Phase != MainLoopPhase::Complete) {
		return false;
	}
	if (session.Pacing.BeginOwned && !SyncDelaySession_Finish(session.Pacing)) {
		return false;
	}

	session.FinishOwned = true;
	return true;
}


bool SyncDelaySession_Begin(SyncDelaySession & session, SyncDelaySessionOps const & ops) noexcept
{
	if (session.BeginOwned || session.FinishOwned || session.Phase != SyncDelayPhase::Idle
		|| !HasValidSyncDelayOps(ops)) {
		return false;
	}

	session.BeginOwned = true;
	session.NetworkWait = ops.IsNetworkWait(ops.Context);
	session.Phase = session.NetworkWait ? SyncDelayPhase::NetworkWait : SyncDelayPhase::LocalWait;
	ops.AccumulateSpareTicks(ops.Context);

	if (ops.TimeRemaining(ops.Context, session.NetworkWait) <= 0) {
		session.Phase = SyncDelayPhase::Complete;
		ops.OnComplete(ops.Context);
	}

	return true;
}


CooperativeResult SyncDelaySession_Step(SyncDelaySession & session, SyncDelaySessionOps const & ops) noexcept
{
	if (!session.BeginOwned || !HasValidSyncDelayOps(ops)) {
		return CooperativeResult::Fault;
	}
	if (session.StepActive) {
		return SetSyncDelayFault(session, FaultReentrantStep);
	}
	if (session.Phase == SyncDelayPhase::Complete) {
		return CooperativeResult::Complete;
	}
	if (session.Phase == SyncDelayPhase::Fault) {
		return CooperativeResult::Fault;
	}
	if (session.Phase != SyncDelayPhase::LocalWait && session.Phase != SyncDelayPhase::NetworkWait) {
		return SetSyncDelayFault(session, FaultInvalidPhase);
	}

	SessionStepGuard active(session.StepActive);
	session.NativeInnerYield = false;
	session.NativeTailYield = false;
	session.NativeTimerCheckInService = false;
	ops.ServiceOnce(ops.Context, session);
	if (ops.TimeRemaining(ops.Context, session.NetworkWait) <= 0) {
		if (session.NativeTimerCheckInService) {
			session.NativeTailYield = false;
		}
		session.Phase = SyncDelayPhase::Complete;
		ops.OnComplete(ops.Context);
		return CooperativeResult::Complete;
	}

	return CooperativeResult::Waiting;
}


bool SyncDelaySession_Finish(SyncDelaySession & session) noexcept
{
	if (session.FinishOwned) {
		return true;
	}
	if (!session.BeginOwned || session.Phase != SyncDelayPhase::Complete) {
		return false;
	}

	session.FinishOwned = true;
	return true;
}

} // namespace OpenTS_MainLoop
