/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "mainloopsession.h"
#include "uiscreen.h"

#include <cstdint>

class UIRmlViewClass;

namespace OpenTS_UIModal
{

enum class UIModalPhase : std::uint8_t
{
	Idle,
	Active,
	Suspended,
	Complete,
	SessionEnded,
	Fault
};

enum class UIModalStepResult : int
{
	Active = 0,
	Waiting = 1,
	Suspended = 2,
	Complete = 3,
	SessionEnded = 4,
	Fault = 5
};

enum class UIModalChildResult : int
{
	Active = 0,
	Waiting = 1,
	Complete = 2,
	SessionEnded = 3,
	Fault = 4
};

// Presenter and view are non-owning references. A caller that yields between Step calls
// remains responsible for keeping both objects alive until Finish.
struct UIModalSession
{
	UIModalPhase Phase = UIModalPhase::Idle;
	UIPresenterClass * Presenter = nullptr;
	UIRmlViewClass * View = nullptr;
	UIResult Result;
	bool BeginOwned = false;
	bool StepActive = false;
	bool RunningModalOwned = false;
	bool InMainLoop = false;
	bool NativeBlockingAdapter = false;
	bool NativeChildEnded = false;
	bool FinishOwned = false;
	int FaultCode = 0;
	OpenTS_MainLoop::MainLoopSession MainLoopChild;
};

// The shell supplies these semantic operations. Tests use the same session machine with
// a synthetic presenter, view and child-frame adapter.
struct UIModalSessionOps
{
	void * Context = nullptr;
	bool (*Ready)(void *) = nullptr;
	void (*EnterRunningModal)(void *) = nullptr;
	void (*LeaveRunningModal)(void *) = nullptr;
	bool (*PresenterResult)(void *, UIResult &) = nullptr;
	bool (*PresenterSuspends)(void *) = nullptr;
	bool (*MainLoopEligible)(void *) = nullptr;
	bool (*MainLoopAlreadyActive)(void *) = nullptr;
	bool (*BeginMainLoop)(void *, UIModalSession &) = nullptr;
	UIModalChildResult (*StepMainLoop)(void *, UIModalSession &) = nullptr;
	bool (*FinishMainLoop)(void *, UIModalSession &) = nullptr;
	void (*CallBack)(void *) = nullptr;
	void (*WinGamepadService)(void *) = nullptr;
	void (*WindowsMessageHandler)(void *) = nullptr;
	void (*ServiceControllerBackAction)(void *) = nullptr;
	void (*PresenterService)(void *) = nullptr;
	void (*ContextUpdate)(void *) = nullptr;
	void (*PresenterDrain)(void *) = nullptr;
	void (*ViewSync)(void *) = nullptr;
	void (*MessageBoxService)(void *) = nullptr;
	void (*MarkDirty)(void *) = nullptr;
	void (*PresentIfDirty)(void *) = nullptr;
};

bool UIModalSession_Begin(
	UIModalSession & session,
	UIPresenterClass & presenter,
	UIRmlViewClass & view,
	bool native_blocking_adapter,
	UIModalSessionOps const & ops) noexcept;

UIModalStepResult UIModalSession_Step(UIModalSession & session, UIModalSessionOps const & ops) noexcept;

bool UIModalSession_Finish(UIModalSession & session, UIModalSessionOps const & ops) noexcept;

} // namespace OpenTS_UIModal
