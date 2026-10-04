/*******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "browsercoord.h"
#include "initgamesession.h"
#include "maingamesession.h"
#include "vqacooperative.h"

#include <array>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace
{

int Failures = 0;
void Check(bool condition, char const * message)
{
	std::printf("%-84s %s\n", message, condition ? "ok" : "FAILED");
	if (!condition) ++Failures;
}

using Action = OpenTS_Browser::StartupAction;
using Result = InitGameStepResult;

struct Fixture
{
	InitGameSession Init;
	MainGameSession Main;
	OpenTS_Browser::BrowserProcessSession Browser;
	VqaCooperativeState Decoder;
	std::array<int, 3> Frames{};
	std::array<int, 3> Begins{};
	std::array<int, 3> Finishes{};
	std::array<int, 3> Completions{};
	std::vector<int> Order;
	int Prepared = 0;
	int Continued = 0;
	int Selector = 0;
	int WalkCalls = 0;
	int Pauses = 0;
	int Resumes = 0;
	int Movie = -1;
	int SkipMovie = -1;
	int MissingMovie = -1;
	int FaultMovie = -1;
	bool Owned = false;
	bool Focus = true;
	bool Paused = false;
	bool ClockReady = true;

	static Result BeginMovie(void * context, unsigned index)
	{
		auto & f = *static_cast<Fixture *>(context);
		Check(!f.Owned, "a new movie cannot replace a live movie resource");
		f.Movie = (int)index;
		f.Owned = true;
		++f.Begins[index];
		f.Order.push_back((int)index);
		f.Decoder = VqaCooperativeState{};
		if (f.FaultMovie == (int)index) return(Result::Fault);
		return(f.MissingMovie == (int)index ? Result::Complete : Result::Waiting);
	}

	static Result StepMovie(void * context)
	{
		auto & f = *static_cast<Fixture *>(context);
		Check(f.Owned, "each movie slice has the same live resource owner");
		if (f.Movie == f.SkipMovie && f.Frames[f.Movie] == 1) return(Result::Complete);
		bool finished = false;
		f.Decoder.Step(f.Focus, true, f.Paused, finished,
			[&](bool & done) {
				++f.WalkCalls;
				if (!f.ClockReady) return(false);
				if (f.Frames[f.Movie] == 4) { done = true; return(false); }
				++f.Frames[f.Movie];
				return(true);
			}, [&]() { ++f.Pauses; f.Paused = true; },
			[&]() { ++f.Resumes; f.Paused = false; });
		if (finished) ++f.Completions[f.Movie];
		return(finished ? Result::Complete : Result::Waiting);
	}

	static void FinishMovie(void * context)
	{
		auto & f = *static_cast<Fixture *>(context);
		Check(f.Owned, "movie resources are released exactly once");
		f.Owned = false;
		++f.Finishes[f.Movie];
	}

	void Begin(void)
	{
		OpenTS_Browser::BrowserProcessOperations browser;
		browser.Context = this;
		browser.Bootstrap = browser.WaitFocus = [](void *) { return(Action::Complete); };
		browser.BeginMainGame = [](void * context) {
			auto & f = *static_cast<Fixture *>(context);
			MainGameSessionOps main;
			main.Context = &f;
			main.InitGame = [](void * context, int, char **) {
				auto & f = *static_cast<Fixture *>(context);
				InitGameSessionOps init;
				init.Context = &f;
				init.Prepare = [](void * context) { ++static_cast<Fixture *>(context)->Prepared; return(0); };
				init.BeginMovie = BeginMovie;
				init.StepMovie = StepMovie;
				init.FinishMovie = FinishMovie;
				init.Complete = [](void * context) {
					auto & f = *static_cast<Fixture *>(context);
					Check(!f.Owned && f.Finishes == std::array<int, 3>{1, 1, 1},
						"caller continuation waits for all three movie lifetimes to end");
					++f.Continued;
					return(0);
				};
				return(f.Init.Begin(init) ? 0 : -1);
			};
			main.StepInitGame = [](void * context) {
				Result const result = static_cast<Fixture *>(context)->Init.Step();
				return(result == Result::Waiting ? MainGameStepResult::Waiting
					: result == Result::Complete ? MainGameStepResult::Complete : MainGameStepResult::Fault);
			};
			main.FinishInitGame = [](void * context) { static_cast<Fixture *>(context)->Init.Finish(); };
			main.BeginSelection = [](void * context, SelectGameSession &, bool) {
				++static_cast<Fixture *>(context)->Selector;
				return(true);
			};
			return(f.Main.Begin(0, nullptr, true, main) ? Action::Waiting : Action::Fault);
		};
		browser.StepMainGame = [](void * context) {
			return(static_cast<Fixture *>(context)->Main.Step() == MainGameStepResult::Waiting
				? Action::Waiting : Action::Fault);
		};
		browser.Finish = [](void * context) { static_cast<Fixture *>(context)->Main.Finish(); };
		Check(OpenTS_Browser::Begin(Browser, browser), "browser process begins with the real MainGame/init coordinators");
	}

	bool Step(void)
	{
		int const before = WalkCalls;
		auto const result = OpenTS_Browser::Step(Browser);
		Check(WalkCalls - before <= 1, "one Browser Step executes at most one decoder WALK");
		return(result == OpenTS_Browser::CooperativeResult::Waiting);
	}

	~Fixture(void) { OpenTS_Browser::Finish(Browser); }
};

void Lifecycle(int skip, int missing)
{
	Fixture f;
	f.SkipMovie = skip;
	f.MissingMovie = missing;
	f.Begin();
	Check(f.Step() && f.Step() && f.Step() && f.Browser.StepCount == 3
		&& f.Browser.Phase == OpenTS_Browser::BrowserProcessPhase::MainGame
		&& f.Main.Phase() == MainGamePhase::InitGame && f.Prepared == 0 && f.WalkCalls == 0,
		"Step 3 returns Waiting after creating initialization, before any movie lifetime");
	for (int i = 0; i < 40 && f.Selector == 0; ++i) Check(f.Step(), "startup slice yields to the browser");
	Check(f.Selector == 1 && f.Prepared == 1 && f.Continued == 1,
		"movie sequence resumes post-init once and then enters the selector");
	Check(f.Order == std::vector<int>{0, 1, 2} && f.Begins == std::array<int, 3>{1, 1, 1},
		"EVA, logo and title sessions retain identity and playback order");
	for (int i = 0; i < 3; ++i) {
		Check(f.Frames[i] == (i == missing ? 0 : i == skip ? 1 : 4),
			"successive Steps advance a retained movie; missing and skipped movies retain their semantics");
		Check(f.Completions[i] == (i == missing || i == skip ? 0 : 1),
			"normal completion is reported exactly once and is distinct from skip");
	}
	f.Main.Finish(); f.Main.Finish(); f.Init.Finish();
	Check(f.Finishes == std::array<int, 3>{1, 1, 1} && !f.Owned,
		"normal, skip, missing-file and repeated parent Finish do not leak or double-free");
	Check(f.Init.Step() == Result::Complete && f.Continued == 1, "completed init is idempotent");
}

void TimingAndAbort(void)
{
	Fixture f;
	f.Begin();
	for (int i = 0; i < 5; ++i) f.Step();
	f.Focus = false;
	for (int i = 0; i < 5; ++i) f.Step();
	Check(f.Pauses == 1 && f.WalkCalls == 0 && f.Owned,
		"lost focus pauses once and yields repeatedly without decoding or releasing the movie");
	f.Focus = true;
	f.ClockReady = false;
	for (int i = 0; i < 4; ++i) f.Step();
	Check(f.Resumes == 1 && f.WalkCalls == 4 && f.Frames[0] == 0,
		"not-time decoder results yield once per Step without faking frame progress");
	f.ClockReady = true;
	f.Step();
	Check(f.Frames[0] == 1 && f.Begins[0] == 1, "focus/timing recovery resumes the same decoder session");
	f.Main.Finish(); f.Main.Finish();
	Check(f.Finishes[0] == 1 && !f.Owned && f.Continued == 0,
		"aborting a live movie releases it once without running startup continuation");

	Fixture failed;
	failed.FaultMovie = 0;
	failed.Begin();
	for (int i = 0; i < 5; ++i) failed.Step();
	failed.Main.Finish(); failed.Main.Finish();
	Check(failed.Finishes[0] == 1 && !failed.Owned && failed.Continued == 0,
		"failed movie initialization still releases partially acquired ownership once");

	VqaCooperativeState state;
	bool done = false;
	int walks = 0;
	state.Step(false, false, false, done, [&](bool &) { ++walks; return(true); }, []() {}, []() {});
	Check(walks == 1, "the existing network focus-loss exception still advances a single slice");
	state.Step(false, false, true, done, [&](bool &) { ++walks; return(true); }, []() {}, []() {});
	state.Step(false, false, true, done, [&](bool &) { ++walks; return(true); }, []() {}, []() {});
	Check(walks == 1 && state.Sleeping, "explicitly paused playback waits for focus, matching the legacy sleeping branch");
	state.Step(true, false, true, done, [&](bool &) { ++walks; return(true); }, []() {}, []() {});
	Check(walks == 2 && !state.Sleeping, "focus restores an explicitly paused decoder without recreating it");
}

std::string Source(char const * path)
{
	std::ifstream file(std::string(OPENTS_SOURCE_ROOT) + "/" + path);
	return(std::string(std::istreambuf_iterator<char>(file), {}));
}

void ProductionWiring(void)
{
	auto const init = Source("code/init.cpp");
	auto const vqa = Source("code/vqa.cpp");
	auto const movie = Source("code/movie.cpp");
	auto const movies = Source("code/movies.cpp");
	auto const startup = Source("code/startup.cpp");
	auto const scenario = Source("code/scenario.cpp");
	auto const restate = Source("code/restate.cpp");
	auto const dropship = Source("code/dropship.cpp");
	auto const slice = vqa.substr(vqa.find("bool VQAClass::Advance_Cooperative_Playback"),
		vqa.find("int VQAClass::Play_VQA") - vqa.find("bool VQAClass::Advance_Cooperative_Playback"));
	Check(slice.find("VQA_Play(Handle, VQAMODE_WALK, 0)") != std::string::npos
		&& slice.find("while") == std::string::npos && slice.find("Host_Sleep") == std::string::npos,
		"production fullscreen decoder uses WALK once with no playback loop or sleep");
	Check(vqa.find("Prepare_Playback(last_frame_to_play);") != std::string::npos
		&& movies.find("Begin_Cooperative_Playback(CooperativePlayback.EndFrame)") != std::string::npos
		&& movies.find("FrameCursor") == std::string::npos,
		"cooperative playback uses the decoder's legacy stop frame instead of a synthetic completion counter");
	Check(init.find("Init_Game_Movies();") != std::string::npos
		&& vqa.find("while (errval >= VQAERR_OK") != std::string::npos
		&& movies.find("Play_VQA(end_frame, user_break_not_allowed)") != std::string::npos,
		"desktop Init_Game / Movie_Play / Play_VQA keep their legacy blocking adapter");
	Check(startup.find("ops.StepInitGame") != std::string::npos
		&& init.find("BrowserStartupMovie->Step()") != std::string::npos
		&& init.find("BrowserStartupMovie.reset()") != std::string::npos,
		"browser MainGame drives the persistent initialization and movie owners");
	for (auto const marker : {"(180)", "(181)", "advanced ? 183 : 182", "(184)", "(185)", "(186)"}) {
		Check(movie.find(marker) != std::string::npos, "native markers distinguish creation, slice, progress, completion, skip and cleanup");
	}
	Check(init.find("OPENTS_BROWSER_STARTUP_OPERATION(187)") != std::string::npos,
		"caller continuation has its own durable native marker");
	auto const scenario_slice = scenario.substr(scenario.find("int ScenarioStartSession::Step"),
		scenario.find("void Pause_Scenario") - scenario.find("int ScenarioStartSession::Step"));
	Check(scenario_slice.find("Play_Movie(") == std::string::npos
		&& scenario_slice.find("Restate_Mission(") == std::string::npos
		&& scenario_slice.find("Dropship_Screen(") == std::string::npos
		&& scenario_slice.find("s.Movie->Step()") != std::string::npos
		&& scenario_slice.find("s.Restatement->Step()") != std::string::npos
		&& scenario_slice.find("s.Dropships->Step()") != std::string::npos,
		"campaign preparation advances persistent movie, briefing and dropship children");
	auto const briefing_slice = restate.substr(restate.find("int RestateMission::StepCooperative"),
		restate.find("void RestateMission::FinishCooperative") - restate.find("int RestateMission::StepCooperative"));
	Check(briefing_slice.find("Wait_") == std::string::npos && briefing_slice.find("while") == std::string::npos
		&& briefing_slice.find("Cooperative_Service_Step(true)") != std::string::npos
		&& dropship.find("while (!accept)") == std::string::npos,
		"adjacent briefing and dropship Steps have no modal lifetime loop");
}

void Preparation(void)
{
	struct State { int Begin = 0, Steps = 0, Finish = 0, Gameplay = 0; } state;
	MainGameSession main;
	MainGameSessionOps ops;
	ops.Context = &state;
	ops.InitGame = [](void *, int, char **) { return(0); };
	ops.StepSelection = [](void *, SelectGameSession & selection) {
		selection.Phase = SelectGamePhase::ScenarioPrepare;
		return(SelectGameStepResult::NativeChild);
	};
	ops.BeginPreparation = [](void * context, SelectGameSession &) { ++static_cast<State *>(context)->Begin; return(true); };
	ops.StepPreparation = [](void * context) {
		auto & s = *static_cast<State *>(context);
		return(++s.Steps < 4 ? MainGameStepResult::Waiting : MainGameStepResult::Complete);
	};
	ops.FinishPreparation = [](void * context) { ++static_cast<State *>(context)->Finish; };
	ops.StepGameplay = [](void * context, OpenTS_MainLoop::MainLoopSession &) {
		++static_cast<State *>(context)->Gameplay;
		return(OpenTS_MainLoop::CooperativeResult::Waiting);
	};
	Check(main.Begin(0, nullptr, true, ops), "campaign parent begins");
	for (int i = 0; i < 6; ++i) Check(main.Step() == MainGameStepResult::Waiting, "preparation returns Waiting to its browser parent");
	Check(state.Begin == 1 && state.Steps == 3 && state.Gameplay == 0 && state.Finish == 0,
		"scenario lifetime is retained across callbacks without stepping deterministic gameplay early");
	Check(main.Step() == MainGameStepResult::Waiting && main.Gameplay_Prepared() && state.Finish == 1,
		"scenario completion cleans its child and enables gameplay exactly once");
	Check(main.Step() == MainGameStepResult::Waiting && state.Gameplay == 1,
		"the existing MainLoop session receives the first gameplay Step after preparation");
	main.Finish(); main.Finish();
	Check(state.Finish == 1, "parent Finish does not release a completed scenario child twice");
}

}

int Test_Cooperative_Movie(void)
{
	Lifecycle(-1, -1);
	Lifecycle(1, -1);
	Lifecycle(-1, 0);
	TimingAndAbort();
	ProductionWiring();
	Preparation();
	return(Failures);
}
