/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "mainloopsession.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#ifndef OPENTS_SOURCE_ROOT
#error OPENTS_SOURCE_ROOT must point at the OpenTS source tree.
#endif

namespace
{

using namespace OpenTS_MainLoop;

int Failures = 0;

void Check(bool condition, char const * description)
{
	std::printf("%-76s %s\n", description, condition ? "ok" : "FAILED");
	if (!condition) {
		++Failures;
	}
}

std::string ReadSource(char const * relative_path)
{
	std::string const path = std::string(OPENTS_SOURCE_ROOT) + "/" + relative_path;
	std::ifstream file(path, std::ios::binary);
	return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

std::size_t Count(std::string const & text, std::string const & needle)
{
	std::size_t count = 0;
	std::size_t offset = 0;
	while ((offset = text.find(needle, offset)) != std::string::npos) {
		++count;
		offset += needle.size();
	}
	return count;
}

struct FakeRuntime
{
	bool Active = true;
	bool Focus = true;
	bool Done = false;
	bool GameEnded = false;
	bool NetworkWait = false;
	int Remaining = 3;
	int Frame = 40;
	int FrameLogicRuns = 0;
	int LogicAI = 0;
	int QueueAI = 0;
	int FocusChecks = 0;
	int FocusServices = 0;
	int EnteredMainLoop = 0;
	int SpareTicksAccumulates = 0;
	int PacingServices = 0;
	int InputServices = 0;
	int RenderServices = 0;
	int TimeChecks = 0;
	int PaceCompletions = 0;
	int DeferredDeletion = 0;
	int SaveServices = 0;
	int BenchmarkEnds = 0;
	int PostFrameCalls = 0;
	bool InMainLoop = false;
	std::vector<std::string> Events;
};

bool IsActive(void * context)
{
	return(static_cast<FakeRuntime *>(context)->Active);
}

void EnterMainLoop(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.EnteredMainLoop;
	runtime.InMainLoop = true;
}

bool FocusReady(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.FocusChecks;
	if (!runtime.Focus) {
		++runtime.FocusServices;
		return(runtime.NetworkWait);
	}
	return(true);
}

bool RunFrameLogic(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.FrameLogicRuns;
	runtime.Events.emplace_back("frame-logic");
	++runtime.LogicAI;
	++runtime.QueueAI;
	++runtime.Frame;
	runtime.Events.emplace_back("frame-increment");
	return(runtime.Done);
}

bool IsNetworkWait(void * context)
{
	return(static_cast<FakeRuntime *>(context)->NetworkWait);
}

void AccumulateSpareTicks(void * context)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.SpareTicksAccumulates;
	runtime.Events.emplace_back("pacing-begin");
}

void ServicePacingOnce(void * context, SyncDelaySession & session)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.PacingServices;
	runtime.Events.emplace_back("pacing-service");
	if (runtime.Focus) {
		++runtime.InputServices;
		++runtime.RenderServices;
	}
	if (session.NetworkWait && runtime.Remaining <= 10) {
		session.NativeInnerYield = true;
	}
}

int TimeRemaining(void * context, bool)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.TimeChecks;
	return(runtime.Remaining);
}

void PacingComplete(void * context)
{
	++static_cast<FakeRuntime *>(context)->PaceCompletions;
}

void PostFrame(void * context, bool done)
{
	FakeRuntime & runtime = *static_cast<FakeRuntime *>(context);
	++runtime.PostFrameCalls;
	if (!done) {
		++runtime.DeferredDeletion;
		++runtime.SaveServices;
	}
	++runtime.BenchmarkEnds;
	runtime.InMainLoop = false;
	runtime.Events.emplace_back("post-frame");
}

bool GameEnded(void * context)
{
	return(static_cast<FakeRuntime *>(context)->GameEnded);
}

MainLoopSessionOps MakeOps(FakeRuntime & runtime)
{
	MainLoopSessionOps ops;
	ops.Context = &runtime;
	ops.GameActive = IsActive;
	ops.EnterMainLoop = EnterMainLoop;
	ops.FocusReady = FocusReady;
	ops.RunFrameLogic = RunFrameLogic;
	ops.Pacing.Context = &runtime;
	ops.Pacing.IsNetworkWait = IsNetworkWait;
	ops.Pacing.AccumulateSpareTicks = AccumulateSpareTicks;
	ops.Pacing.ServiceOnce = ServicePacingOnce;
	ops.Pacing.TimeRemaining = TimeRemaining;
	ops.Pacing.OnComplete = PacingComplete;
	ops.PostFrame = PostFrame;
	ops.GameEnded = GameEnded;
	return(ops);
}

void Test_One_Frame_And_Pacing(void)
{
	FakeRuntime runtime;
	MainLoopSession session;
	MainLoopSessionOps const ops = MakeOps(runtime);

	Check(MainLoopSession_Begin(session), "MainLoopSession Begin acquires one frame owner");
	Check(!MainLoopSession_Begin(session), "duplicate MainLoopSession Begin is rejected");
	Check(MainLoopSession_Step(session, ops) == CooperativeResult::Waiting,
		"one bounded Step runs one semantic frame and returns while pacing is pending");
	Check(session.Phase == MainLoopPhase::Pace && runtime.Frame == 41
		&& runtime.FrameLogicRuns == 1 && runtime.LogicAI == 1 && runtime.QueueAI == 1,
		"one semantic frame increments Frame once and runs AI and Queue_AI once");
	Check(runtime.PacingServices == 1 && runtime.InputServices == 1 && runtime.RenderServices == 1,
		"the first pacing step services callback, input and render once");
	Check(runtime.Events.size() >= 4 && runtime.Events[0] == "frame-logic"
		&& runtime.Events[1] == "frame-increment" && runtime.Events[2] == "pacing-begin"
		&& runtime.Events[3] == "pacing-service",
		"semantic frame completes before the pacing child begins");
	Check(runtime.PostFrameCalls == 0 && runtime.DeferredDeletion == 0 && runtime.SaveServices == 0,
		"post-frame ownership waits until pacing completes");

	runtime.Remaining = 2;
	Check(MainLoopSession_Step(session, ops) == CooperativeResult::Waiting,
		"a later pacing callback returns Waiting while the timer remains active");
	Check(runtime.Frame == 41 && runtime.FrameLogicRuns == 1 && runtime.LogicAI == 1
		&& runtime.QueueAI == 1 && runtime.InputServices == 2 && runtime.RenderServices == 2,
		"pacing may service input/render without another frame, AI or queue pass");

	runtime.Remaining = 0;
	Check(MainLoopSession_Step(session, ops) == CooperativeResult::Complete,
		"timer completion reaches the post-frame phase and completes the session");
	Check(session.Phase == MainLoopPhase::Complete && runtime.PaceCompletions == 1,
		"SyncDelaySession completes once and MainLoopSession reports Complete");
	Check(runtime.PostFrameCalls == 1 && runtime.DeferredDeletion == 1
		&& runtime.SaveServices == 1 && runtime.BenchmarkEnds == 1 && !runtime.InMainLoop,
		"deferred deletion, save service, benchmark finish and loop cleanup each run once");
	Check(MainLoopSession_Step(session, ops) == CooperativeResult::Complete
		&& runtime.FrameLogicRuns == 1 && runtime.PacingServices == 3
		&& runtime.PostFrameCalls == 1,
		"Step after completion does not repeat frame, pacing or post-frame work");
	Check(MainLoopSession_Finish(session) && MainLoopSession_Finish(session),
		"Finish closes the completed main-loop and pacing sessions idempotently");
}

void Test_Focus_Wait(void)
{
	FakeRuntime runtime;
	runtime.Focus = false;
	runtime.Remaining = 0;
	MainLoopSession session;
	MainLoopSessionOps const ops = MakeOps(runtime);
	MainLoopSession_Begin(session);

	Check(MainLoopSession_Step(session, ops) == CooperativeResult::Waiting,
		"cooperative focus loss yields Waiting instead of blocking");
	Check(runtime.FocusChecks == 1 && runtime.FocusServices == 1
		&& runtime.FrameLogicRuns == 0 && runtime.Frame == 40,
		"focus wait services once and does not start a semantic frame");

	runtime.Focus = true;
	Check(MainLoopSession_Step(session, ops) == CooperativeResult::Complete,
		"the next host callback resumes after focus returns");
	Check(runtime.FrameLogicRuns == 1 && runtime.Frame == 41 && runtime.LogicAI == 1
		&& runtime.QueueAI == 1 && runtime.PostFrameCalls == 1,
		"focus resumption runs one frame and one post-frame completion");
	Check(MainLoopSession_Finish(session), "focused session finishes cleanly");
}

void Test_Network_Focus_Continuation(void)
{
	FakeRuntime runtime;
	runtime.Focus = false;
	runtime.NetworkWait = true;
	runtime.Remaining = 0;
	MainLoopSession session;
	MainLoopSessionOps const ops = MakeOps(runtime);
	MainLoopSession_Begin(session);

	Check(MainLoopSession_Step(session, ops) == CooperativeResult::Complete,
		"non-local session pumps focus once and preserves its continue-while-unfocused path");
	Check(runtime.FocusChecks == 1 && runtime.FocusServices == 1
		&& runtime.FrameLogicRuns == 1 && runtime.Frame == 41,
		"non-local focus pump does not introduce a persistent wait state");
	Check(MainLoopSession_Finish(session), "non-local focus continuation finishes cleanly");
}

void Test_Network_SyncDelay_State(void)
{
	FakeRuntime runtime;
	runtime.NetworkWait = true;
	runtime.Remaining = 11;
	SyncDelaySession session;
	SyncDelaySessionOps ops;
	ops.Context = &runtime;
	ops.IsNetworkWait = IsNetworkWait;
	ops.AccumulateSpareTicks = AccumulateSpareTicks;
	ops.ServiceOnce = ServicePacingOnce;
	ops.TimeRemaining = TimeRemaining;
	ops.OnComplete = PacingComplete;

	Check(SyncDelaySession_Begin(session, ops)
		&& session.Phase == SyncDelayPhase::NetworkWait,
		"SyncDelaySession begins in its native network-pacing phase");
	Check(runtime.SpareTicksAccumulates == 1 && runtime.TimeChecks == 1,
		"pacing accounting and initial timer check happen once at Begin");
	Check(SyncDelaySession_Step(session, ops) == CooperativeResult::Waiting
		&& runtime.PacingServices == 1 && !session.NativeInnerYield,
		"network pacing services once and preserves the native early-wait distinction");
	runtime.Remaining = 10;
	Check(SyncDelaySession_Step(session, ops) == CooperativeResult::Waiting
		&& runtime.PacingServices == 2 && session.NativeInnerYield,
		"near-deadline network pacing records the native zero-wait yield hint");
	runtime.Remaining = 0;
	Check(SyncDelaySession_Step(session, ops) == CooperativeResult::Complete
		&& runtime.PaceCompletions == 1,
		"network pacing completes once when its timer expires");
	Check(SyncDelaySession_Finish(session), "completed SyncDelaySession finishes cleanly");
}

void Test_Production_Source_Boundaries(void)
{
	std::string const mainloop = ReadSource("code/mainloop.cpp");
	std::string const session = ReadSource("code/mainloopsession.cpp");
	std::string const cmake = ReadSource("code/CMakeLists.txt");
	std::string const harness = ReadSource("tests/browsercoord/harness.js");
	std::size_t const post_frame_start = mainloop.find("static void MainLoop_Post_Frame");
	std::size_t const post_frame_end = mainloop.find("static bool MainLoop_Game_Ended", post_frame_start);
	std::size_t const focus_start = mainloop.find("static bool MainLoop_Focus_Ready");
	std::size_t const focus_end = mainloop.find("static bool MainLoop_Run_Frame", focus_start);
	std::string const focus = focus_start != std::string::npos && focus_end != std::string::npos
		? mainloop.substr(focus_start, focus_end - focus_start)
		: std::string();
	std::string const post_frame = post_frame_start != std::string::npos
		&& post_frame_end != std::string::npos
		? mainloop.substr(post_frame_start, post_frame_end - post_frame_start)
		: std::string();

	Check(session.find("Host_Sleep") == std::string::npos,
		"cooperative MainLoopSession and SyncDelaySession Step code contains no Host_Sleep");
	Check(mainloop.find("MainLoopSession_Begin(session)") != std::string::npos
		&& mainloop.find("MainLoopSession_Step(session, ops)") != std::string::npos
		&& mainloop.find("MainLoopSession_Finish(session)") != std::string::npos,
		"native Main_Loop uses the shared cooperative Begin/Step/Finish core");
	Check(mainloop.find("static void Native_Sync_Delay_Wait") != std::string::npos
		&& mainloop.find("Host_Sleep(Video_Milliseconds_Until_Present())") != std::string::npos
		&& mainloop.find("Host_Sleep(16 * FrameTimer.Value())") != std::string::npos,
		"native blocking pacing retains its presentation and frame-timer waits");
	Check(Count(mainloop, "Logic.AI();") == 1 && Count(mainloop, "Queue_AI();") == 1
		&& Count(mainloop, "Frame++;") == 1,
		"production source keeps one AI, Queue_AI and Frame increment site in frame logic");
	Check(Count(post_frame, "Process_Deferred_Deletion();") == 1
		&& Count(post_frame, "SaveManager.Service();") == 1
		&& Count(post_frame, "BEnd(BENCH_GAME_FRAME);") == 1
		&& Count(post_frame, "InMainLoop = false;") == 1,
		"production source has one post-frame owner for deletion, save, benchmark and cleanup");
	Check(focus.find("Windows_Message_Handler();") != std::string::npos
		&& focus.find("Session.Type != GAME_NORMAL && Session.Type != GAME_SKIRMISH") != std::string::npos,
		"focus handling waits cooperatively for local play and preserves non-local single-pump behavior");
	Check(cmake.find("ASYNCIFY") == std::string::npos && cmake.find("JSPI") == std::string::npos,
		"Emscripten link configuration remains free of Asyncify and JSPI");
	Check(harness.find("MainLoopPhase") == std::string::npos
		&& harness.find("SyncDelayPhase") == std::string::npos
		&& harness.find("Logic.AI") == std::string::npos
		&& harness.find("Queue_AI") == std::string::npos,
		"the browser harness carries no deterministic frame or pacing semantics");
}

} // namespace

int main(void)
{
	Test_One_Frame_And_Pacing();
	Test_Focus_Wait();
	Test_Network_Focus_Continuation();
	Test_Network_SyncDelay_State();
	Test_Production_Source_Boundaries();
	std::printf("\n%s\n", Failures == 0 ? "PASSED" : "FAILED");
	return(Failures == 0 ? 0 : 1);
}
