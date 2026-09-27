/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "ui/uimodalsession.h"

namespace
{

enum UIModalFaultCode
{
	FaultInvalidOperations = 1,
	FaultStepBeforeBegin = 2,
	FaultReentrantStep = 3,
	FaultChildBegin = 4,
	FaultChildStep = 5,
	FaultChildFinish = 6,
	FaultFinishBeforeTerminal = 7
};

bool HasValidOps(OpenTS_UIModal::UIModalSessionOps const & ops)
{
	return(ops.Ready != nullptr
		&& ops.EnterRunningModal != nullptr
		&& ops.LeaveRunningModal != nullptr
		&& ops.PresenterResult != nullptr
		&& ops.PresenterSuspends != nullptr
		&& ops.MainLoopEligible != nullptr
		&& ops.MainLoopAlreadyActive != nullptr
		&& ops.BeginMainLoop != nullptr
		&& ops.StepMainLoop != nullptr
		&& ops.FinishMainLoop != nullptr
		&& ops.CallBack != nullptr
		&& ops.WinGamepadService != nullptr
		&& ops.WindowsMessageHandler != nullptr
		&& ops.ServiceControllerBackAction != nullptr
		&& ops.PresenterService != nullptr
		&& ops.ContextUpdate != nullptr
		&& ops.PresenterDrain != nullptr
		&& ops.ViewSync != nullptr
		&& ops.MessageBoxService != nullptr
		&& ops.MarkDirty != nullptr
		&& ops.PresentIfDirty != nullptr);
}

void SetFault(OpenTS_UIModal::UIModalSession & session, int code)
{
	session.Phase = OpenTS_UIModal::UIModalPhase::Fault;
	session.FaultCode = code;
}

bool IsTerminal(OpenTS_UIModal::UIModalPhase phase)
{
	using OpenTS_UIModal::UIModalPhase;
	return(phase == UIModalPhase::Suspended
		|| phase == UIModalPhase::Complete
		|| phase == UIModalPhase::SessionEnded
		|| phase == UIModalPhase::Fault);
}

OpenTS_UIModal::UIModalStepResult ToStepResult(OpenTS_UIModal::UIModalPhase phase)
{
	using OpenTS_UIModal::UIModalPhase;
	using OpenTS_UIModal::UIModalStepResult;
	switch (phase) {
		case UIModalPhase::Active: return(UIModalStepResult::Active);
		case UIModalPhase::Suspended: return(UIModalStepResult::Suspended);
		case UIModalPhase::Complete: return(UIModalStepResult::Complete);
		case UIModalPhase::SessionEnded: return(UIModalStepResult::SessionEnded);
		case UIModalPhase::Fault: return(UIModalStepResult::Fault);
		case UIModalPhase::Idle: return(UIModalStepResult::Fault);
	}
	return(UIModalStepResult::Fault);
}

class StepGuard
{
	public:
		explicit StepGuard(bool & active) : Active(active) { Active = true; }
		~StepGuard() { Active = false; }
	private:
		bool & Active;
};

} // namespace

namespace OpenTS_UIModal
{

bool UIModalSession_Begin(
	UIModalSession & session,
	UIPresenterClass & presenter,
	UIRmlViewClass & view,
	bool native_blocking_adapter,
	UIModalSessionOps const & ops) noexcept
{
	if (session.BeginOwned || session.FinishOwned || session.Phase != UIModalPhase::Idle) {
		return(false);
	}
	if (!HasValidOps(ops)) {
		SetFault(session, FaultInvalidOperations);
		return(false);
	}

	session.Presenter = &presenter;
	session.View = &view;
	session.Result = UIResult{};
	session.Result.Outcome = UIResult::OUTCOME_CANCELLED;
	session.NativeBlockingAdapter = native_blocking_adapter;
	session.BeginOwned = true;

	if (!ops.Ready(ops.Context)) {
		session.Result.Outcome = UIResult::OUTCOME_FAILED_TO_OPEN;
		session.Phase = UIModalPhase::Complete;
		return(true);
	}

	ops.EnterRunningModal(ops.Context);
	session.RunningModalOwned = true;
	session.Phase = UIModalPhase::Active;
	return(true);
}


UIModalStepResult UIModalSession_Step(UIModalSession & session, UIModalSessionOps const & ops) noexcept
{
	if (!session.BeginOwned) {
		SetFault(session, FaultStepBeforeBegin);
		return(UIModalStepResult::Fault);
	}
	if (session.StepActive) {
		SetFault(session, FaultReentrantStep);
		return(UIModalStepResult::Fault);
	}
	if (IsTerminal(session.Phase)) {
		return(ToStepResult(session.Phase));
	}
	if (session.Phase != UIModalPhase::Active || !HasValidOps(ops)) {
		SetFault(session, FaultInvalidOperations);
		return(UIModalStepResult::Fault);
	}

	StepGuard active(session.StepActive);
	UIResult result;
	if (ops.PresenterResult(ops.Context, result)) {
		session.Result = result;
		session.Phase = UIModalPhase::Complete;
		return(UIModalStepResult::Complete);
	}
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);
	if (ops.PresenterSuspends(ops.Context)) {
		session.Phase = UIModalPhase::Suspended;
		return(UIModalStepResult::Suspended);
	}
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);

	ops.WinGamepadService(ops.Context);
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);
	ops.WindowsMessageHandler(ops.Context);
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);
	ops.ServiceControllerBackAction(ops.Context);
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);

	bool const main_loop_eligible = ops.MainLoopEligible(ops.Context);
	bool const another_main_loop_active = !session.InMainLoop
		&& main_loop_eligible
		&& ops.MainLoopAlreadyActive(ops.Context);
	bool const main_loop_route = session.InMainLoop || (main_loop_eligible && !another_main_loop_active);
	UIModalChildResult child_result = UIModalChildResult::Active;
	if (main_loop_route) {
		if (!session.InMainLoop) {
			if (!ops.BeginMainLoop(ops.Context, session)) {
				SetFault(session, FaultChildBegin);
				return(UIModalStepResult::Fault);
			}
			session.InMainLoop = true;
		}
		child_result = ops.StepMainLoop(ops.Context, session);
		if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);
		if (child_result == UIModalChildResult::Fault) {
			SetFault(session, FaultChildStep);
			return(UIModalStepResult::Fault);
		}
		if (child_result == UIModalChildResult::Complete || child_result == UIModalChildResult::SessionEnded) {
			bool const child_finished = ops.FinishMainLoop(ops.Context, session);
			session.InMainLoop = false;
			if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);
			if (!child_finished) {
				SetFault(session, FaultChildFinish);
				return(UIModalStepResult::Fault);
			}
			if (child_result == UIModalChildResult::SessionEnded) {
				session.Result.Outcome = UIResult::OUTCOME_SESSION_ENDED;
				session.Result.GameEnded = true;
				session.Phase = UIModalPhase::SessionEnded;
				return(UIModalStepResult::SessionEnded);
			}
		}
	} else if (!main_loop_eligible) {
		ops.CallBack(ops.Context);
		if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);
	}

	ops.PresenterService(ops.Context);
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);
	ops.ContextUpdate(ops.Context);
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);
	ops.PresenterDrain(ops.Context);
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);
	ops.ViewSync(ops.Context);
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);
	ops.MessageBoxService(ops.Context);
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);
	ops.MarkDirty(ops.Context);
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);
	ops.PresentIfDirty(ops.Context);
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);

	if (ops.PresenterResult(ops.Context, result)) {
		session.Result = result;
		session.Phase = UIModalPhase::Complete;
		return(UIModalStepResult::Complete);
	}
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);
	if (ops.PresenterSuspends(ops.Context)) {
		session.Phase = UIModalPhase::Suspended;
		return(UIModalStepResult::Suspended);
	}
	if (session.Phase == UIModalPhase::Fault) return(UIModalStepResult::Fault);

	return(child_result == UIModalChildResult::Waiting
		? UIModalStepResult::Waiting
		: UIModalStepResult::Active);
}


bool UIModalSession_Finish(UIModalSession & session, UIModalSessionOps const & ops) noexcept
{
	if (session.FinishOwned) {
		return(true);
	}
	if (!session.BeginOwned) {
		return(false);
	}

	bool success = true;
	if (session.Phase == UIModalPhase::Active) {
		SetFault(session, FaultFinishBeforeTerminal);
		success = false;
	}
	if (session.InMainLoop) {
		if (HasValidOps(ops)) {
			success = ops.FinishMainLoop(ops.Context, session) && success;
		} else {
			success = false;
		}
		session.InMainLoop = false;
	}
	if (session.RunningModalOwned) {
		if (ops.LeaveRunningModal != nullptr) {
			ops.LeaveRunningModal(ops.Context);
		} else {
			success = false;
		}
		session.RunningModalOwned = false;
	}
	session.FinishOwned = true;
	return(success);
}

} // namespace OpenTS_UIModal
