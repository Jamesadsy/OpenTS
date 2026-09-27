/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "mainloopsession.h"
#include "ui/uimodalsession.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
#include <vector>

#ifndef OPENTS_SOURCE_ROOT
#error OPENTS_SOURCE_ROOT must point at the OpenTS source tree.
#endif

namespace
{

using namespace OpenTS_UIModal;
using namespace OpenTS_MainLoop;

int Failures = 0;

void Check(bool condition, char const * description)
{
	std::printf("%-76s %s\n", description, condition ? "ok" : "FAILED");
	if (!condition) ++Failures;
}

std::string ReadSource(char const * relative_path)
{
	std::string const path = std::string(OPENTS_SOURCE_ROOT) + "/" + relative_path;
	std::ifstream file(path, std::ios::binary);
	return(std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()));
}

std::size_t Count(std::string const & text, std::string const & needle)
{
	std::size_t count = 0;
	std::size_t offset = 0;
	while ((offset = text.find(needle, offset)) != std::string::npos) {
		++count;
		offset += needle.size();
	}
	return(count);
}

std::string ExtractFunction(std::string const & source, char const * signature)
{
	std::size_t const start = source.find(signature);
	if (start == std::string::npos) return(std::string());
	std::size_t const open = source.find('{', start);
	if (open == std::string::npos) return(std::string());
	int depth = 0;
	for (std::size_t i = open; i < source.size(); ++i) {
		if (source[i] == '{') ++depth;
		if (source[i] == '}' && --depth == 0) return(source.substr(start, i - start + 1));
	}
	return(std::string());
}

class FakePresenter : public UIPresenterClass
{
	public:
		void Execute(UIIntent const &) override {}
		void Refresh(void) override {}
};

struct FakeRuntime
{
	FakePresenter Presenter;
	int ViewMarker = 0;
	bool Ready = true;
	bool Suspended = false;
	bool SuspendOnDrain = false;
	bool SetResultOnDrain = false;
	UIResult DrainResult;
	bool FrameEligible = false;
	bool ExternalMainLoopActive = false;
	bool FrameActive = true;
	bool FrameDone = false;
	bool Focus = true;
	int Remaining = 2;
	int Frame = 40;
	int LogicAI = 0;
	int QueueAI = 0;
	int ModalDepth = 0;
	int EnterCount = 0;
	int LeaveCount = 0;
	int CallbackCount = 0;
	int PresenterServiceCount = 0;
	int ContextUpdateCount = 0;
	int DrainCount = 0;
	int ViewSyncCount = 0;
	int MessageBoxCount = 0;
	int DirtyCount = 0;
	int PresentCount = 0;
	int FrameLogicCount = 0;
	int FrameBeginCount = 0;
	int FrameFinishCount = 0;
	int PacingServiceCount = 0;
	int PacingCompleteCount = 0;
	int PostFrameCount = 0;
	int SpareTicksCount = 0;
	int MainLoopEnterCount = 0;
	int FocusCheckCount = 0;
	bool InMainLoop = false;
	std::vector<std::string> Events;
};

void Event(FakeRuntime & runtime, char const * name)
{
	runtime.Events.emplace_back(name);
}

bool FrameGameActive(void * context)
{
	return(static_cast<FakeRuntime *>(context)->FrameActive);
}

void FrameEnter(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.MainLoopEnterCount;
	runtime.InMainLoop = true;
	Event(runtime, "frame-enter");
}

bool FrameFocusReady(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.FocusCheckCount;
	return(runtime.Focus);
}

bool FrameLogic(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.FrameLogicCount;
	++runtime.Frame;
	++runtime.LogicAI;
	++runtime.QueueAI;
	Event(runtime, "frame-logic");
	return(runtime.FrameDone);
}

bool PacingIsNetworkWait(void *)
{
	return(false);
}

void PacingAccumulate(void * context)
{
	++static_cast<FakeRuntime *>(context)->SpareTicksCount;
}

void PacingService(void * context, SyncDelaySession &)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.PacingServiceCount;
	Event(runtime, "frame-pacing");
	if (runtime.Remaining > 0) --runtime.Remaining;
}

int PacingRemaining(void * context, bool)
{
	return(static_cast<FakeRuntime *>(context)->Remaining);
}

void PacingComplete(void * context)
{
	++static_cast<FakeRuntime *>(context)->PacingCompleteCount;
}

void FramePost(void * context, bool)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.PostFrameCount;
	runtime.InMainLoop = false;
	Event(runtime, "frame-post");
}

bool FrameEnded(void * context)
{
	return(!static_cast<FakeRuntime *>(context)->FrameActive);
}

MainLoopSessionOps MakeFrameOps(FakeRuntime & runtime)
{
	MainLoopSessionOps ops;
	ops.Context = &runtime;
	ops.GameActive = FrameGameActive;
	ops.EnterMainLoop = FrameEnter;
	ops.FocusReady = FrameFocusReady;
	ops.RunFrameLogic = FrameLogic;
	ops.Pacing.Context = &runtime;
	ops.Pacing.IsNetworkWait = PacingIsNetworkWait;
	ops.Pacing.AccumulateSpareTicks = PacingAccumulate;
	ops.Pacing.ServiceOnce = PacingService;
	ops.Pacing.TimeRemaining = PacingRemaining;
	ops.Pacing.OnComplete = PacingComplete;
	ops.PostFrame = FramePost;
	ops.GameEnded = FrameEnded;
	return(ops);
}

bool Ready(void * context)
{
	return(static_cast<FakeRuntime *>(context)->Ready);
}

void EnterRunningModal(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.ModalDepth;
	++runtime.EnterCount;
	Event(runtime, "modal-enter");
}

void LeaveRunningModal(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	--runtime.ModalDepth;
	++runtime.LeaveCount;
	Event(runtime, "modal-leave");
}

bool PresenterResult(void * context, UIResult & result)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	if (!runtime.Presenter.Result.has_value()) return(false);
	result = runtime.Presenter.Result.value();
	return(true);
}

bool PresenterSuspends(void * context)
{
	return(static_cast<FakeRuntime *>(context)->Suspended);
}

bool MainLoopEligible(void * context)
{
	return(static_cast<FakeRuntime *>(context)->FrameEligible);
}

bool MainLoopAlreadyActive(void * context)
{
	return(static_cast<FakeRuntime *>(context)->ExternalMainLoopActive);
}

bool BeginMainLoop(void * context, UIModalSession & session)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.FrameBeginCount;
	return(MainLoopSession_Begin(session.MainLoopChild));
}

UIModalChildResult StepMainLoop(void * context, UIModalSession & session)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	CooperativeResult const result = MainLoopSession_Step(session.MainLoopChild, MakeFrameOps(runtime));
	switch (result) {
		case CooperativeResult::Active: return(UIModalChildResult::Active);
		case CooperativeResult::Waiting: return(UIModalChildResult::Waiting);
		case CooperativeResult::Complete:
			return(session.MainLoopChild.Result ? UIModalChildResult::SessionEnded : UIModalChildResult::Complete);
		case CooperativeResult::Fault: return(UIModalChildResult::Fault);
	}
	return(UIModalChildResult::Fault);
}

bool FinishMainLoop(void * context, UIModalSession & session)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.FrameFinishCount;
	bool const success = MainLoopSession_Finish(session.MainLoopChild);
	if (success) session.MainLoopChild = MainLoopSession{};
	return(success);
}

void CallBack(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.CallbackCount;
	Event(runtime, "callback");
}

void WinGamepadService(void * context)
{
	Event(*static_cast<FakeRuntime *>(context), "gamepad");
}

void WindowsMessageHandler(void * context)
{
	Event(*static_cast<FakeRuntime *>(context), "messages");
}

void ServiceControllerBackAction(void * context)
{
	Event(*static_cast<FakeRuntime *>(context), "controller-back");
}

void PresenterService(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.PresenterServiceCount;
	Event(runtime, "presenter-service");
}

void ContextUpdate(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.ContextUpdateCount;
	Event(runtime, "context-update");
}

void PresenterDrain(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.DrainCount;
	Event(runtime, "presenter-drain");
	if (runtime.SuspendOnDrain) runtime.Suspended = true;
	if (runtime.SetResultOnDrain) runtime.Presenter.Result = runtime.DrainResult;
}

void ViewSync(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.ViewSyncCount;
	Event(runtime, "view-sync");
}

void MessageBoxService(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.MessageBoxCount;
	Event(runtime, "message-box");
}

void MarkDirty(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.DirtyCount;
	Event(runtime, "dirty");
}

void PresentIfDirty(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.PresentCount;
	Event(runtime, "present");
}

UIModalSessionOps MakeModalOps(FakeRuntime & runtime)
{
	UIModalSessionOps ops;
	ops.Context = &runtime;
	ops.Ready = Ready;
	ops.EnterRunningModal = EnterRunningModal;
	ops.LeaveRunningModal = LeaveRunningModal;
	ops.PresenterResult = PresenterResult;
	ops.PresenterSuspends = PresenterSuspends;
	ops.MainLoopEligible = MainLoopEligible;
	ops.MainLoopAlreadyActive = MainLoopAlreadyActive;
	ops.BeginMainLoop = BeginMainLoop;
	ops.StepMainLoop = StepMainLoop;
	ops.FinishMainLoop = FinishMainLoop;
	ops.CallBack = CallBack;
	ops.WinGamepadService = WinGamepadService;
	ops.WindowsMessageHandler = WindowsMessageHandler;
	ops.ServiceControllerBackAction = ServiceControllerBackAction;
	ops.PresenterService = PresenterService;
	ops.ContextUpdate = ContextUpdate;
	ops.PresenterDrain = PresenterDrain;
	ops.ViewSync = ViewSync;
	ops.MessageBoxService = MessageBoxService;
	ops.MarkDirty = MarkDirty;
	ops.PresentIfDirty = PresentIfDirty;
	return(ops);
}

bool Begin(UIModalSession & session, FakeRuntime & runtime, bool native_blocking = false)
{
	UIRmlViewClass * const view = reinterpret_cast<UIRmlViewClass *>(&runtime.ViewMarker);
	return(UIModalSession_Begin(session, runtime.Presenter, *view, native_blocking, MakeModalOps(runtime)));
}

void TestOnePassAndResultOwnership(void)
{
	FakeRuntime runtime;
	UIResult expected;
	expected.Outcome = UIResult::OUTCOME_ACCEPTED;
	expected.Value = 19;
	expected.GameEnded = false;
	runtime.SetResultOnDrain = true;
	runtime.DrainResult = expected;

	UIModalSession session;
	UIModalSessionOps ops = MakeModalOps(runtime);
	UIRmlViewClass * const view = reinterpret_cast<UIRmlViewClass *>(&runtime.ViewMarker);
	Check(UIModalSession_Begin(session, runtime.Presenter, *view, false, ops), "Begin establishes one native modal session");
	Check(session.Presenter == &runtime.Presenter && session.View == view, "session keeps non-owning presenter and view references");
	Check(runtime.ModalDepth == 1 && runtime.EnterCount == 1, "Begin acquires _RunningModal exactly once");
	UIModalStepResult const step = UIModalSession_Step(session, ops);
	Check(step == UIModalStepResult::Complete, "one bounded modal pass completes when presenter returns a result");
	Check(session.Result.Outcome == expected.Outcome && session.Result.Value == 19 && !session.Result.GameEnded,
		"presenter UIResult outcome and value remain native-owned and are copied intact");
	std::vector<std::string> const expected_events = {
		"modal-enter", "gamepad", "messages", "controller-back", "callback", "presenter-service",
		"context-update", "presenter-drain", "view-sync", "message-box", "dirty", "present"
	};
	Check(runtime.Events == expected_events, "one Step executes one modal pass in the required semantic order");
	int const event_count = static_cast<int>(runtime.Events.size());
	Check(UIModalSession_Step(session, ops) == UIModalStepResult::Complete
		&& static_cast<int>(runtime.Events.size()) == event_count,
		"repeated Step after Complete performs no second pass");
	Check(UIModalSession_Finish(session, ops) && runtime.ModalDepth == 0 && runtime.LeaveCount == 1,
		"Finish releases modal-depth ownership exactly once");
	Check(UIModalSession_Finish(session, ops) && runtime.ModalDepth == 0 && runtime.LeaveCount == 1,
		"repeated Finish is idempotent");
}

void TestSuspensionAndSessionEnded(void)
{
	{
		FakeRuntime runtime;
		runtime.SuspendOnDrain = true;
		UIModalSession session;
		UIModalSessionOps ops = MakeModalOps(runtime);
		Check(Begin(session, runtime), "suspension case begins");
		Check(UIModalSession_Step(session, ops) == UIModalStepResult::Suspended,
			"presenter suspension yields control after its current semantic pass");
		Check(session.Presenter == &runtime.Presenter && session.View != nullptr && session.Result.Outcome == UIResult::OUTCOME_CANCELLED,
			"suspension retains the parent references and legacy cancelled result without closing the parent");
		int const drain_count = runtime.DrainCount;
		Check(UIModalSession_Step(session, ops) == UIModalStepResult::Suspended && runtime.DrainCount == drain_count,
			"repeated Step while suspended performs no duplicate presenter drain");
		Check(UIModalSession_Finish(session, ops) && runtime.ModalDepth == 0 && runtime.LeaveCount == 1,
			"suspended parent releases _RunningModal once when its adapter finishes");
	}

	{
		FakeRuntime runtime;
		runtime.FrameEligible = true;
		runtime.FrameActive = false;
		UIModalSession session;
		UIModalSessionOps ops = MakeModalOps(runtime);
		Check(Begin(session, runtime), "session-ended case begins");
		Check(UIModalSession_Step(session, ops) == UIModalStepResult::SessionEnded,
			"124B child completion reports session-ended without presenter work");
		Check(session.Result.Outcome == UIResult::OUTCOME_SESSION_ENDED && session.Result.GameEnded,
			"session-ended UIResult outcome and GameEnded value are preserved");
		Check(runtime.PresenterServiceCount == 0 && runtime.DrainCount == 0 && runtime.FrameFinishCount == 1,
			"session-ended path finishes its child once and skips later modal operations");
		int const event_count = static_cast<int>(runtime.Events.size());
		Check(UIModalSession_Step(session, ops) == UIModalStepResult::SessionEnded
			&& static_cast<int>(runtime.Events.size()) == event_count,
			"repeated Step after SessionEnded performs no semantic work");
		Check(UIModalSession_Finish(session, ops) && runtime.ModalDepth == 0,
			"session-ended Finish balances _RunningModal once");
	}
}

void TestChildFrameResumptionAndModalDepth(void)
{
	FakeRuntime runtime;
	runtime.FrameEligible = true;
	UIModalSession session;
	UIModalSessionOps ops = MakeModalOps(runtime);
	Check(Begin(session, runtime), "124B child-frame case begins");
	Check(UIModalSession_Step(session, ops) == UIModalStepResult::Waiting,
		"first modal Step resumes one bounded 124B frame/pacing child operation");
	Check(runtime.Frame == 41 && runtime.FrameLogicCount == 1 && runtime.LogicAI == 1 && runtime.QueueAI == 1
		&& runtime.PacingServiceCount == 1 && session.InMainLoop && runtime.InMainLoop,
		"first child Step runs one semantic frame and preserves in-main-loop state while pacing waits");
	Check(UIModalSession_Step(session, ops) == UIModalStepResult::Active,
		"second modal Step resumes the existing 124B pacing child and returns to its parent pass");
	Check(runtime.Frame == 41 && runtime.FrameLogicCount == 1 && runtime.LogicAI == 1 && runtime.QueueAI == 1
		&& runtime.PacingServiceCount == 2 && runtime.PostFrameCount == 1 && !session.InMainLoop && !runtime.InMainLoop,
		"pacing resumption does not duplicate Frame, AI, Queue_AI, or post-frame ownership");
	Check(runtime.FrameBeginCount == 1 && runtime.FrameFinishCount == 1 && runtime.ModalDepth == 1,
		"one child session and one modal depth owner span both bounded passes");
	UIResult result;
	result.Outcome = UIResult::OUTCOME_ACCEPTED;
	result.Value = 7;
	runtime.Presenter.Result = result;
	Check(UIModalSession_Step(session, ops) == UIModalStepResult::Complete,
		"modal parent accepts the result after the child frame finishes");
	int const event_count = static_cast<int>(runtime.Events.size());
	Check(UIModalSession_Step(session, ops) == UIModalStepResult::Complete
		&& static_cast<int>(runtime.Events.size()) == event_count,
		"completed child modal does not restart its frame or pacing on repeated Step");
	Check(UIModalSession_Finish(session, ops) && runtime.ModalDepth == 0 && runtime.LeaveCount == 1,
		"child-frame parent Finish balances modal depth exactly once");
}

void TestAlreadyActiveFrameGuardAndInvalidCalls(void)
{
	{
		FakeRuntime runtime;
		runtime.FrameEligible = true;
		runtime.ExternalMainLoopActive = true;
		UIModalSession session;
		UIModalSessionOps ops = MakeModalOps(runtime);
		Check(Begin(session, runtime), "nested-frame guard case begins");
		Check(UIModalSession_Step(session, ops) == UIModalStepResult::Active,
			"nested modal pass returns while an outer 124B frame owns the loop");
		Check(runtime.FrameBeginCount == 0 && runtime.CallbackCount == 0 && runtime.FrameLogicCount == 0,
			"nested pass neither duplicates Main_Loop nor calls its competing Call_Back path");
		Check(UIModalSession_Finish(session, ops) == false,
			"active session cannot be silently finished before a terminal result");
		Check(runtime.ModalDepth == 0 && runtime.LeaveCount == 1,
			"faulted early Finish still balances its owned modal depth");
	}

	{
		FakeRuntime runtime;
		UIModalSession session;
		UIModalSessionOps ops = MakeModalOps(runtime);
		Check(UIModalSession_Step(session, ops) == UIModalStepResult::Fault,
			"Step before Begin fails safely");
		Check(runtime.ModalDepth == 0 && runtime.EnterCount == 0,
			"Step before Begin does not acquire modal depth");
	}

	{
		FakeRuntime runtime;
		runtime.Ready = false;
		UIModalSession session;
		UIModalSessionOps ops = MakeModalOps(runtime);
		UIRmlViewClass * const view = reinterpret_cast<UIRmlViewClass *>(&runtime.ViewMarker);
		Check(UIModalSession_Begin(session, runtime.Presenter, *view, false, ops),
			"failed-to-open Begin establishes a terminal session without modal-depth ownership");
		Check(UIModalSession_Step(session, ops) == UIModalStepResult::Complete
			&& session.Result.Outcome == UIResult::OUTCOME_FAILED_TO_OPEN && runtime.ModalDepth == 0,
			"failed-to-open result remains compatible and does not decrement an unowned depth");
		Check(UIModalSession_Finish(session, ops) && runtime.LeaveCount == 0,
			"failed-to-open Finish is safe without modal-depth ownership");
	}
}

void TestSourceBoundaries(void)
{
	std::string const core = ReadSource("code/ui/uimodalsession.cpp");
	std::string const shell = ReadSource("code/ui/uishell.cpp");
	std::string const adapter = ExtractFunction(shell, "UIResult UI_Run_Modal(");
	std::string const emscripten_config = ReadSource("CMakePresets.json") + ReadSource("code/CMakeLists.txt");
	Check(!core.empty() && core.find("Host_Sleep") == std::string::npos,
		"cooperative UIModalSession core contains no Host_Sleep");
	Check(adapter.find("UI_Modal_Session_Step") != std::string::npos
		&& adapter.find("Host_Sleep(Video_Milliseconds_Until_Present())") != std::string::npos
		&& adapter.find("UI_Modal_Session_Finish") != std::string::npos,
		"native blocking UI_Run_Modal drives the same session and keeps waiting outside Step");
	Check(shell.find("Main_Loop_Session_Begin") != std::string::npos
		&& shell.find("Main_Loop_Session_Step") != std::string::npos
		&& shell.find("Main_Loop_Session_Finish") != std::string::npos,
		"InMainLoop child path delegates to the accepted 124B session API");
	Check(emscripten_config.find("ASYNCIFY") == std::string::npos
		&& emscripten_config.find("JSPI") == std::string::npos,
		"Emscripten configuration retains disabled Asyncify and JSPI");
	Check(Count(core, "UIModalSession_Step") >= 1,
		"source includes the generic native Begin/Step/Finish session core");
}

} // namespace

int main(void)
{
	TestOnePassAndResultOwnership();
	TestSuspensionAndSessionEnded();
	TestChildFrameResumptionAndModalDepth();
	TestAlreadyActiveFrameGuardAndInvalidCalls();
	TestSourceBoundaries();
	return(Failures == 0 ? 0 : 1);
}
