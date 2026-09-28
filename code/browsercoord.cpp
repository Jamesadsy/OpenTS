/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "browsercoord.h"

#if defined(__EMSCRIPTEN__)
extern "C" int OpenTS_Browser_Startup_Bootstrap(void);
extern "C" int OpenTS_Browser_Startup_WaitFocus(void);
extern "C" int OpenTS_Browser_Startup_Begin_MainGame(void);
extern "C" int OpenTS_Browser_Startup_Step_MainGame(void);
extern "C" int OpenTS_Browser_Startup_Shutdown_Request(void);
extern "C" int OpenTS_Browser_Startup_Shutdown_Drain(void);
extern "C" void OpenTS_Browser_Startup_Finish(void);
extern "C" int OpenTS_Browser_Startup_OwnerData_Boundary(void);
extern "C" int OpenTS_Browser_Startup_Renderer_Initialized(void);
extern "C" int OpenTS_Browser_Startup_Bootstrap_Stage(void);
extern "C" int OpenTS_Browser_Startup_Finish_Stage(void);
extern "C" int OpenTS_Browser_Startup_ProgEnd_Stage(void);
extern "C" char const * OpenTS_Browser_Startup_Renderer_Name(void);
#endif

namespace
{

OpenTS_Browser::BrowserProcessSession Session;
int BeginCount = 0;
int FinishCount = 0;
int BeginResult = 0;
int MainReturned = 0;

constexpr int FaultInvalidPhase = 1;
constexpr int FaultMissingOperation = 2;
constexpr int FaultReentrantStep = 3;
constexpr int FaultNativeChild = 4;
constexpr int FaultUnexpectedAction = 5;

#if defined(__EMSCRIPTEN__)
using OpenTS_Browser::StartupAction;

StartupAction Bootstrap(void *)
{
	return(OpenTS_Browser_Startup_Bootstrap() == 1 ? StartupAction::Complete : StartupAction::Fault);
}

StartupAction WaitFocus(void *)
{
	int const result = OpenTS_Browser_Startup_WaitFocus();
	return(result == 1 ? StartupAction::Complete : result == 0 ? StartupAction::Waiting : StartupAction::Fault);
}

StartupAction BeginMainGame(void *)
{
	int const result = OpenTS_Browser_Startup_Begin_MainGame();
	return(result == 1 ? StartupAction::Boundary : result == 0 ? StartupAction::Waiting : StartupAction::Fault);
}

StartupAction StepMainGame(void *)
{
	int const result = OpenTS_Browser_Startup_Step_MainGame();
	switch (result) {
		case 0: return(StartupAction::Waiting);
		case 1: return(StartupAction::Complete);
		case 2: return(StartupAction::NativeChild);
		default: return(StartupAction::Fault);
	}
}

StartupAction ShutdownRequest(void *)
{
	return(OpenTS_Browser_Startup_Shutdown_Request() == 1 ? StartupAction::Complete : StartupAction::Fault);
}

StartupAction ShutdownDrain(void *)
{
	int const result = OpenTS_Browser_Startup_Shutdown_Drain();
	return(result == 1 ? StartupAction::Complete : result == 0 ? StartupAction::Waiting : StartupAction::Fault);
}

void FinishStartup(void *)
{
	OpenTS_Browser_Startup_Finish();
}

OpenTS_Browser::BrowserProcessOperations ProductionOperations()
{
	OpenTS_Browser::BrowserProcessOperations operations;
	operations.Bootstrap = &Bootstrap;
	operations.WaitFocus = &WaitFocus;
	operations.BeginMainGame = &BeginMainGame;
	operations.StepMainGame = &StepMainGame;
	operations.ShutdownRequest = &ShutdownRequest;
	operations.ShutdownDrain = &ShutdownDrain;
	operations.Finish = &FinishStartup;
	return(operations);
}

StartupAction StorageProofFault(void *)
{
	return(StartupAction::Fault);
}

void StorageProofFinish(void * context)
{
	++(*static_cast<int *>(context));
}
#endif

struct StepActivityGuard
{
	explicit StepActivityGuard(OpenTS_Browser::BrowserProcessSession & session) noexcept
		: Session(session)
	{
		Session.StepActive = true;
	}

	~StepActivityGuard() noexcept
	{
		Session.StepActive = false;
	}

	OpenTS_Browser::BrowserProcessSession & Session;
};

void SetFault(OpenTS_Browser::BrowserProcessSession & session, int code) noexcept
{
	session.Phase = OpenTS_Browser::BrowserProcessPhase::Fault;
	session.FaultCode = code;
}

OpenTS_Browser::StartupAction RunAction(
	OpenTS_Browser::BrowserProcessSession & session,
	OpenTS_Browser::StartupAction (*action)(void *)) noexcept
{
	if (action == nullptr) {
		SetFault(session, FaultMissingOperation);
		return(OpenTS_Browser::StartupAction::Fault);
	}
	return(action(session.Operations.Context));
}

} // namespace

namespace OpenTS_Browser
{

bool Begin(BrowserProcessSession & session, BrowserProcessOperations const & operations) noexcept
{
	if (session.BeginOwned || session.FinishOwned || session.Phase != BrowserProcessPhase::Idle) {
		return(false);
	}

	session.Operations = operations;
	session.StepCount = 0;
	session.WaitFocusYields = 0;
	session.MainGameSteps = 0;
	session.ShutdownDrainPasses = 0;
	session.FaultCode = 0;
	session.BeginOwned = true;
	session.MainGameBeginOwned = false;
	session.Phase = BrowserProcessPhase::Bootstrap;
	return(true);
}


CooperativeResult Step(BrowserProcessSession & session) noexcept
{
	if (!session.BeginOwned) {
		return(CooperativeResult::Fault);
	}

	if (session.StepActive) {
		SetFault(session, FaultReentrantStep);
		return(CooperativeResult::Fault);
	}

	if (session.Phase == BrowserProcessPhase::Complete) {
		return(CooperativeResult::Complete);
	}
	if (session.Phase == BrowserProcessPhase::Fault) {
		return(CooperativeResult::Fault);
	}

	StepActivityGuard active(session);
	++session.StepCount;

	using Phase = BrowserProcessPhase;
	using Action = StartupAction;
	Action result = Action::Fault;
	switch (session.Phase) {
		case Phase::Idle:
			SetFault(session, FaultInvalidPhase);
			return(CooperativeResult::Fault);

		case Phase::Bootstrap:
			result = RunAction(session, session.Operations.Bootstrap);
			if (result == Action::Complete) {
				session.Phase = Phase::WaitFocus;
				return(CooperativeResult::Waiting);
			}
			break;

		case Phase::WaitFocus:
			result = RunAction(session, session.Operations.WaitFocus);
			if (result == Action::Waiting) {
				++session.WaitFocusYields;
				return(CooperativeResult::Waiting);
			}
			if (result == Action::Complete) {
				session.Phase = Phase::MainGame;
				return(CooperativeResult::Waiting);
			}
			break;

		case Phase::MainGame:
			if (!session.MainGameBeginOwned) {
				session.MainGameBeginOwned = true;
				result = RunAction(session, session.Operations.BeginMainGame);
				if (result == Action::Boundary) {
					session.Phase = Phase::ShutdownRequest;
					return(CooperativeResult::Waiting);
				}
				if (result == Action::Waiting) {
					return(CooperativeResult::Waiting);
				}
				if (result == Action::Complete) {
					session.Phase = Phase::ShutdownRequest;
					return(CooperativeResult::Waiting);
				}
			} else {
				++session.MainGameSteps;
				result = RunAction(session, session.Operations.StepMainGame);
				if (result == Action::Waiting) {
					return(CooperativeResult::Waiting);
				}
				if (result == Action::Complete) {
					session.Phase = Phase::ShutdownRequest;
					return(CooperativeResult::Waiting);
				}
				if (result == Action::NativeChild) {
					SetFault(session, FaultNativeChild);
					return(CooperativeResult::Fault);
				}
			}
			break;

		case Phase::ShutdownRequest:
			result = RunAction(session, session.Operations.ShutdownRequest);
			if (result == Action::Complete) {
				session.Phase = Phase::ShutdownDrain;
				return(CooperativeResult::Waiting);
			}
			break;

		case Phase::ShutdownDrain:
			++session.ShutdownDrainPasses;
			result = RunAction(session, session.Operations.ShutdownDrain);
			if (result == Action::Waiting) {
				return(CooperativeResult::Waiting);
			}
			if (result == Action::Complete) {
				session.Phase = Phase::Complete;
				return(CooperativeResult::Complete);
			}
			break;

		case Phase::Complete:
			return(CooperativeResult::Complete);

		case Phase::Fault:
			return(CooperativeResult::Fault);
	}

	if (session.Phase != Phase::Fault) {
		SetFault(session, result == Action::Fault ? FaultUnexpectedAction : FaultInvalidPhase);
	}
	return(CooperativeResult::Fault);
}


BrowserProcessPhase Status(BrowserProcessSession const & session) noexcept
{
	return(session.Phase);
}


bool Finish(BrowserProcessSession & session) noexcept
{
	if (session.FinishOwned) {
		return(true);
	}
	if (!session.BeginOwned
		|| (session.Phase != BrowserProcessPhase::Complete && session.Phase != BrowserProcessPhase::Fault)) {
		return(false);
	}

	session.FinishOwned = true;
	if (session.Operations.Finish != nullptr) {
		session.Operations.Finish(session.Operations.Context);
	}
	return(true);
}

} // namespace OpenTS_Browser

extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Begin(void)
{
	++BeginCount;
#if defined(__EMSCRIPTEN__)
	if (BeginCount != 1) return(0);
	BeginResult = OpenTS_Browser::Begin(Session, ProductionOperations()) ? 1 : 0;
	return(BeginResult);
#else
	return(0);
#endif
}

extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Step(void)
{
	return(static_cast<int>(OpenTS_Browser::Step(Session)));
}

extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Status(void)
{
	return(static_cast<int>(OpenTS_Browser::Status(Session)));
}

extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Finish(void)
{
	if (!OpenTS_Browser::Finish(Session)) return(0);
	++FinishCount;
	return(1);
}

extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Begin_Count(void) { return(BeginCount); }
extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Begin_Result(void) { return(BeginResult); }
extern "C" OPENTS_BROWSER_EXPORT void OpenTS_Browser_Main_Entry_Returned(void) { MainReturned = 1; }
extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Main_Returned(void) { return(MainReturned); }
extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Step_Count(void) { return((int)Session.StepCount); }
extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_WaitFocus_Yields(void) { return((int)Session.WaitFocusYields); }
extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_MainGame_Began(void) { return(Session.MainGameBeginOwned ? 1 : 0); }
extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_MainGame_Steps(void) { return((int)Session.MainGameSteps); }
extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Shutdown_Drain_Passes(void) { return((int)Session.ShutdownDrainPasses); }

extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_OwnerData_Boundary(void)
{
#if defined(__EMSCRIPTEN__)
	return(OpenTS_Browser_Startup_OwnerData_Boundary());
#else
	return(0);
#endif
}

extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Fault_Code(void) { return(Session.FaultCode); }
extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Finish_Count(void) { return(FinishCount); }

extern "C" OPENTS_BROWSER_EXPORT int OpenTS_124H_Browser_Fault_Finish_Proof(void)
{
#if defined(__EMSCRIPTEN__)
	OpenTS_Browser::BrowserProcessSession proofSession;
	int finishCalls = 0;
	OpenTS_Browser::BrowserProcessOperations operations;
	operations.Bootstrap = &StorageProofFault;
	operations.Finish = &StorageProofFinish;
	operations.Context = &finishCalls;
	if (!OpenTS_Browser::Begin(proofSession, operations)) return(0);
	auto const stepResult = OpenTS_Browser::Step(proofSession);
	int proof = 0;
	if (stepResult == OpenTS_Browser::CooperativeResult::Fault) proof |= 1;
	if (OpenTS_Browser::Status(proofSession) == OpenTS_Browser::BrowserProcessPhase::Fault) proof |= 2;
	if (proofSession.FaultCode == FaultUnexpectedAction) proof |= 4;
	bool const firstFinish = OpenTS_Browser::Finish(proofSession);
	bool const repeatedFinish = OpenTS_Browser::Finish(proofSession);
	if (firstFinish && repeatedFinish && finishCalls == 1) proof |= 8;
	return(proof);
#else
	return(0);
#endif
}

extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Renderer_Initialized(void)
{
#if defined(__EMSCRIPTEN__)
	return(OpenTS_Browser_Startup_Renderer_Initialized());
#else
	return(0);
#endif
}

extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Bootstrap_Stage(void)
{
#if defined(__EMSCRIPTEN__)
	return(OpenTS_Browser_Startup_Bootstrap_Stage());
#else
	return(0);
#endif
}

extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_Finish_Stage(void)
{
#if defined(__EMSCRIPTEN__)
	return(OpenTS_Browser_Startup_Finish_Stage());
#else
	return(0);
#endif
}

extern "C" OPENTS_BROWSER_EXPORT int OpenTS_Browser_ProgEnd_Stage(void)
{
#if defined(__EMSCRIPTEN__)
	return(OpenTS_Browser_Startup_ProgEnd_Stage());
#else
	return(0);
#endif
}

extern "C" OPENTS_BROWSER_EXPORT char const * OpenTS_Browser_Renderer_Name(void)
{
#if defined(__EMSCRIPTEN__)
	return(OpenTS_Browser_Startup_Renderer_Name());
#else
	return("");
#endif
}
