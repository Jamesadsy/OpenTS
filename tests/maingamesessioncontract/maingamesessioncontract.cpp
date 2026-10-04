/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "maingamesession.h"
#include "globals.h"
#include "special.hh"
#include "scenario.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string>

#ifndef OPENTS_SOURCE_ROOT
#error OPENTS_SOURCE_ROOT must point at the OpenTS source tree.
#endif

SpecialDialogType SpecialDialog = SDLG_NONE;
bool GameActive = true;
bool ScenarioActive = true;
bool TacticalActive = true;
int Frame = 0;
ScenarioClass * Scen = nullptr;

struct SelectGameSession::Impl {};
SelectGameSession::SelectGameSession(void) = default;
SelectGameSession::~SelectGameSession(void) = default;
void SelectGameSession::Begin(bool fade, bool)
{
	Fade = fade;
	Phase = SelectGamePhase::MainMenu;
}
void SelectGameSession::Reset(void) { Phase = SelectGamePhase::Idle; }
SelectGameStepResult SelectGameSession::Step(void) { return(SelectGameStepResult::Waiting); }
SelectGameLaterBoundary SelectGameSession::Later_Boundary(void) const
{
	return(SelectGameLaterBoundary::None);
}
bool SelectGameSession::Complete_Native_Child(bool, int, bool, bool)
{
	Phase = SelectGamePhase::ScenarioPrepare;
	return(true);
}

struct IngameMenuSession::Impl {};
IngameMenuSession::IngameMenuSession(void) = default;
IngameMenuSession::~IngameMenuSession(void) = default;
void IngameMenuSession::Begin(bool) {}
IngameMenuStepResult IngameMenuSession::Step(void) noexcept { return(IngameMenuStepResult::Complete); }
bool IngameMenuSession::Finish(void) noexcept { return(true); }
IngameMenuPhase IngameMenuSession::Phase(void) const noexcept { return(IngameMenuPhase::Complete); }

int Init_Game(int, char **)
{
	return(0);
}

bool Main_Loop_Session_Begin(OpenTS_MainLoop::MainLoopSession &) { return(true); }
OpenTS_MainLoop::CooperativeResult Main_Loop_Session_Step(OpenTS_MainLoop::MainLoopSession &)
{
	return(OpenTS_MainLoop::CooperativeResult::Complete);
}
bool Main_Loop_Session_Finish(OpenTS_MainLoop::MainLoopSession &) { return(true); }

namespace
{
	int Failures = 0;

	void Check(bool condition, char const * description)
	{
		std::printf("%-84s %s\n", description, condition ? "ok" : "FAILED");
		if (!condition) ++Failures;
	}

	std::string ReadSource(char const * relative_path)
	{
		std::string const path = std::string(OPENTS_SOURCE_ROOT) + "/" + relative_path;
		std::ifstream file(path, std::ios::binary);
		return(std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()));
	}

	std::string ExtractBlock(std::string const & source, char const * begin, char const * end)
	{
	std::size_t const first = source.find(begin);
		if (first == std::string::npos) return(std::string{});
	std::size_t const last = source.find(end, first + std::string(begin).size());
		if (last == std::string::npos) return(std::string{});
		return(source.substr(first, last - first));
	}

	std::size_t Count(std::string const & text, char const * needle)
	{
		std::size_t count = 0;
		std::size_t offset = 0;
		while ((offset = text.find(needle, offset)) != std::string::npos) {
			++count;
			offset += std::strlen(needle);
		}
		return(count);
	}

	bool InOrder(std::string const & text, std::initializer_list<char const *> needles)
	{
		std::size_t offset = 0;
		for (char const * needle : needles) {
			offset = text.find(needle, offset);
			if (offset == std::string::npos) return(false);
			offset += std::strlen(needle);
		}
		return(true);
	}

	struct Runtime
	{
		int InitCalls = 0;
		int SelectionBegins = 0;
		int SelectionSteps = 0;
		int PreparationCompletions = 0;
		int GameplayBegins = 0;
		int GameplaySteps = 0;
		int GameplayFinishes = 0;
		int ModalPendingCalls = 0;
		int ModalBegins = 0;
		int ModalSteps = 0;
		int ModalFinishes = 0;
		int CleanupCalls = 0;
		int FrontendReturns = 0;
		bool ModalActive = true;
		SelectGameSession * SelectionAddress = nullptr;
	};

	int Init(void * context, int, char **)
	{
		++static_cast<Runtime *>(context)->InitCalls;
		return(0);
	}

	bool BeginSelection(void * context, SelectGameSession & selection, bool fade)
	{
		Runtime & runtime = *static_cast<Runtime *>(context);
		++runtime.SelectionBegins;
		runtime.SelectionAddress = &selection;
		selection.Begin(fade);
		return(true);
	}

	SelectGameStepResult StepSelection(void * context, SelectGameSession & selection)
	{
		Runtime & runtime = *static_cast<Runtime *>(context);
		++runtime.SelectionSteps;
		Check(runtime.SelectionAddress == &selection, "SelectGameSession identity is retained across parent Steps");
		if (runtime.SelectionSteps == 1) {
			selection.Phase = SelectGamePhase::NativeChild;
			return(SelectGameStepResult::NativeChild);
		}
		selection.Phase = SelectGamePhase::ReadyForGameplay;
		return(SelectGameStepResult::ReadyForGameplay);
	}

	bool BeginGameplay(void * context, OpenTS_MainLoop::MainLoopSession & loop)
	{
		++static_cast<Runtime *>(context)->GameplayBegins;
		loop.LoopStarted = true;
		return(true);
	}

	OpenTS_MainLoop::CooperativeResult StepGameplay(void * context, OpenTS_MainLoop::MainLoopSession & loop)
	{
		Runtime & runtime = *static_cast<Runtime *>(context);
		++runtime.GameplaySteps;
		if (runtime.GameplaySteps == 1) return(OpenTS_MainLoop::CooperativeResult::Waiting);
		if (runtime.GameplaySteps == 3) loop.Result = true;
		return(OpenTS_MainLoop::CooperativeResult::Complete);
	}

	bool FinishGameplay(void * context, OpenTS_MainLoop::MainLoopSession &)
	{
		++static_cast<Runtime *>(context)->GameplayFinishes;
		return(true);
	}

	bool ModalPending(void * context)
	{
		Runtime & runtime = *static_cast<Runtime *>(context);
		++runtime.ModalPendingCalls;
		return(runtime.ModalActive);
	}

	bool BeginModal(void * context, IngameMenuSession &)
	{
		++static_cast<Runtime *>(context)->ModalBegins;
		return(true);
	}

	IngameMenuStepResult StepModal(void * context, IngameMenuSession &)
	{
		Runtime & runtime = *static_cast<Runtime *>(context);
		++runtime.ModalSteps;
		return(runtime.ModalSteps == 1 ? IngameMenuStepResult::Waiting : IngameMenuStepResult::Complete);
	}

	bool FinishModal(void * context, IngameMenuSession &)
	{
		Runtime & runtime = *static_cast<Runtime *>(context);
		++runtime.ModalFinishes;
		runtime.ModalActive = false;
		return(true);
	}

	bool Cleanup(void * context)
	{
		++static_cast<Runtime *>(context)->CleanupCalls;
		return(true);
	}

	bool ReturnFrontend(void * context, bool fade)
	{
		Runtime & runtime = *static_cast<Runtime *>(context);
		++runtime.FrontendReturns;
		return(!fade);
	}

	MainGameSessionOps MakeOps(Runtime & runtime)
	{
		MainGameSessionOps ops;
		ops.Context = &runtime;
		ops.InitGame = Init;
		ops.BeginSelection = BeginSelection;
		ops.StepSelection = StepSelection;
		ops.BeginGameplay = BeginGameplay;
		ops.StepGameplay = StepGameplay;
		ops.FinishGameplay = FinishGameplay;
		ops.ModalPending = ModalPending;
		ops.BeginModal = BeginModal;
		ops.StepModal = StepModal;
		ops.FinishModal = FinishModal;
		ops.CleanupGameplay = Cleanup;
		ops.ReturnFrontend = ReturnFrontend;
		return(ops);
	}

	void Test_MainGameSession_Lifecycle(char const * campaign)
	{
		std::printf("Live MainLoop continuation regression: %s campaign\n", campaign);
		Runtime runtime;
		MainGameSession session;
		MainGameSessionOps const ops = MakeOps(runtime);
		char * argv[] = {nullptr};

		Check(session.Begin(0, argv, false, ops) && runtime.InitCalls == 1,
			"MainGameSession owns Init_Game exactly once at Begin");
		Check(session.Step() == MainGameStepResult::Waiting
			&& session.Phase() == MainGamePhase::SelectGame,
			"selection parent begins once and remains owned by MainGameSession");
		Check(session.Step() == MainGameStepResult::NativeChild
			&& runtime.SelectionSteps == 1,
			"SelectGame native child is handed to its caller without replacing its session");
		Check(session.Complete_Selection_Native_Child(true, 0, true)
			&& session.Selection()->Phase == SelectGamePhase::ScenarioPrepare,
			"native selection child result returns to the retained SelectGameSession");
		Check(session.Step() == MainGameStepResult::NativeChild
			&& session.Phase() == MainGamePhase::PrepareGameplay,
			"accepted selection reaches a caller-owned gameplay preparation seam");
		Check(session.Complete_Gameplay_Preparation(true)
			&& !session.Complete_Gameplay_Preparation(true)
			&& session.Gameplay_Prepared(),
			"successful gameplay preparation is accepted once");
		Check(session.Step() == MainGameStepResult::Waiting
			&& session.Phase() == MainGamePhase::IngameModal
			&& runtime.GameplaySteps == 0,
			"modal chain starts before MainLoopSession and prevents gameplay simulation");
		Check(session.Step() == MainGameStepResult::Waiting
			&& runtime.GameplaySteps == 0,
			"active modal child yields without stepping MainLoopSession");
		Check(session.Step() == MainGameStepResult::Waiting
			&& session.Phase() == MainGamePhase::Gameplay
			&& runtime.ModalBegins == 1 && runtime.ModalFinishes == 1,
			"modal chain finishes once before gameplay can resume");
		Check(runtime.GameplaySteps == 0
			&& session.Step() == MainGameStepResult::Waiting
			&& runtime.GameplaySteps == 1,
			"the accepted MainLoopSession becomes the sole gameplay path after modal completion");
		Check(session.Step() == MainGameStepResult::Waiting
			&& runtime.GameplaySteps == 2 && runtime.GameplayFinishes == 1
			&& session.Phase() == MainGamePhase::Gameplay && runtime.CleanupCalls == 0,
			"a completed live native frame is finished and resumed without mission cleanup");
		Check(session.Step() == MainGameStepResult::Waiting
			&& runtime.GameplaySteps == 3 && runtime.GameplayBegins == 2
			&& runtime.GameplayFinishes == 2 && session.Phase() == MainGamePhase::GameCleanup,
			"native GameActive end result, rather than a frame boundary, starts mission cleanup");
		Check(session.Step() == MainGameStepResult::Waiting
			&& runtime.CleanupCalls == 1 && session.Cleanup_Owned() && session.Fade(),
			"game-end cleanup runs once and preserves frontend return fade semantics");
		Check(session.Step() == MainGameStepResult::Complete
			&& runtime.FrontendReturns == 1,
			"ReturnFrontend is applied exactly once after cleanup");
		Check(session.Step() == MainGameStepResult::Complete
			&& runtime.InitCalls == 1 && runtime.SelectionBegins == 1
			&& runtime.GameplaySteps == 3 && runtime.CleanupCalls == 1
			&& runtime.FrontendReturns == 1,
			"terminal MainGameSession Steps are idempotent");
		Check(session.Finish() && session.Finish(), "MainGameSession Finish is idempotent");
	}

	void Test_Production_Seams(void)
	{
		std::string const main_game = ReadSource("code/maingamesession.cpp");
		std::string const in_game = ReadSource("code/conquer.cpp");
		std::string const netshare = ReadSource("code/netshare.cpp");
		std::string const mapgen = ReadSource("code/mapgen.cpp");
		std::string const options = ReadSource("code/ui/uigameoptions.cpp");
		std::string const controls = ReadSource("code/gamedlg.cpp");
		std::string const save = ReadSource("code/ui/uisavebrowser.cpp");
		std::string const mapgen_ui = ReadSource("code/ui/uimapgen.cpp");
		std::string const scenario = ReadSource("code/ui/uiscenariopick.cpp");
		std::string const skirmish = ReadSource("code/ui/uiskirmish.cpp");
		std::string const nested = ReadSource("code/ui/uifrontendcontinuation.h");
		std::string const random_map_setup = ExtractBlock(netshare,
			"std::unique_ptr<UIFrontendContinuation> Pick_Scenario_Screen_Continuation(",
			"/// <summary>");
		std::string const ingame_step = ExtractBlock(in_game, "IngameMenuStepResult IngameMenuSession::Step(void)", "bool IngameMenuSession::Finish(void)");
		std::string const transition_step = ExtractBlock(ingame_step,
			"case IngameMenuPhase::ApplyChildResult:", "case IngameMenuPhase::Resume:");
		std::string const random_map_child = ExtractBlock(netshare,
			"class RandomMapCreationContinuation", "} // namespace");
		std::string const mapgen_cleanup = ExtractBlock(mapgen,
			"void Cleanup(void) noexcept", "UIMapGenPresenterClass m_Presenter;");
		std::string const scenario_child = ExtractBlock(scenario,
			"UIScenarioPickPresenterClass::Start_Child_Continuation(",
			"void UIScenarioPickPresenterClass::Execute(");
		std::string const skirmish_child = ExtractBlock(skirmish,
			"UISkirmishPresenterClass::Start_Child_Continuation(",
			"/// <summary>");
		std::string const save_children = ExtractBlock(save,
			"UISaveBrowserPresenterClass::Start_Child_Continuation(",
			"/// <summary>");
		std::string const child_flow = ExtractBlock(nested,
			"if (m_Child != nullptr)", "UIModalStepResult const parent_step");
		std::string const parent_flow = ExtractBlock(nested,
			"UIModalStepResult const parent_step", "UIResult const & Result(void) const noexcept override");
		std::string const controls_apply = ExtractBlock(controls,
			"class GameControlsDialogContinuation", "} // namespace");

		Check(main_game.find("Host_Sleep") == std::string::npos
			&& ingame_step.find("Host_Sleep") == std::string::npos
			&& ingame_step.find("while (") == std::string::npos,
			"cooperative MainGameSession and IngameMenuSession Steps contain no Host_Sleep or busy loop");
		Check(main_game.find("m_Impl->Ingame.Begin(false)") != std::string::npos
			&& main_game.find("m_Impl->Ingame.Step()") != std::string::npos,
			"MainGameSession owns one persistent IngameMenuSession across modal Steps");
		Check(random_map_setup.find("Session.Type != GAME_SKIRMISH") != std::string::npos,
			"ScenarioPick continuation is restricted to local game types");
		Check(netshare.find("Pick_Scenario_Screen_Continuation") != std::string::npos
			&& netshare.find("CreateRandomMap_Continuation") != std::string::npos
			&& mapgen.find("Do_Random_Map_Dialog_Continuation") != std::string::npos,
			"Skirmish and ScenarioPick retain native child continuations through random-map generation");
		Check(mapgen.find("if (m_Cleaned) return") != std::string::npos
			&& mapgen.find("Write_PCX_File") != std::string::npos
			&& mapgen.find("delete RandomMapGen.MapSeeder") != std::string::npos
			&& netshare.find("Register_Random_Map_Scenario()") != std::string::npos,
			"MapGen continuation records once-only write/finalise and resource cleanup ownership");
		Check(options.find("Start_Child_Continuation") != std::string::npos
			&& in_game.find("GameControls_Dialog_Continuation") != std::string::npos
			&& controls.find("m_Applied") != std::string::npos
			&& controls.find("Options.Save_Settings()") != std::string::npos,
			"SETTINGS retains the GameControls child and applies native options once");
		Check(save.find("ChildAction") != std::string::npos
			&& save.find("CHILD_OVERWRITE_CONFIRM") != std::string::npos
			&& save.find("CHILD_SAVE_WRITE") != std::string::npos
			&& save.find("UI_Message_Box_Screen_Continuation") != std::string::npos,
			"SaveBrowser queues ordered confirmation/result message children");
		Check(in_game.find("Pause_Scenario") != std::string::npos
			&& in_game.find("Resume_Scenario") != std::string::npos
			&& in_game.find("SDLG_NONE") != std::string::npos,
			"SpecialDialog remains native authority for one pause/resume modal chain");
		Check(Count(ingame_step, "Pause_Scenario();") == 1
			&& Count(ingame_step, "Resume_Scenario();") == 1
			&& ingame_step.find("!m_Impl->PauseOwned") != std::string::npos
			&& ingame_step.find("!m_Impl->ResumeOwned && SpecialDialog == SDLG_NONE") != std::string::npos,
			"Pause_Scenario and Resume_Scenario each have one guarded Step site, with resume gated on NONE");
		Check(InOrder(ingame_step, {"case IngameMenuPhase::Pause:", "Pause_Scenario();",
			"case IngameMenuPhase::Dispatch:", "case IngameMenuPhase::ChildActive:",
			"case IngameMenuPhase::ApplyChildResult:", "case IngameMenuPhase::Resume:"}),
			"the in-game chain pauses before dispatch and resumes only after child-result application");
		Check(transition_step.find("SpecialDialog = SDLG_NONE") != std::string::npos
			&& transition_step.find("SpecialDialog = SDLG_OPTIONS") != std::string::npos
			&& transition_step.find("SpecialDialog = SDLG_SETTINGS") != std::string::npos
			&& transition_step.find("PlayerRestarts = true") != std::string::npos
			&& transition_step.find("PlayerPtr->Flag_To_Lose()") != std::string::npos
			&& transition_step.find("QuickloadError") != std::string::npos,
			"OPTIONS, SETTINGS/SOUND/KEYBOARD, ABORT, SURRENDER and QUICKLOAD transitions stay in C++");
		Check(InOrder(parent_flow, {"m_Parent->Step()", "m_Parent->ViewPtr()->Hide()",
			"m_Parent->FinishModalOnly()", "provider->Start_Child_Continuation"})
			&& InOrder(child_flow, {"m_Child->Step()", "m_Child->Finish()",
			"provider->Complete_Child_Continuation", "m_Parent->ViewPtr()->Show()",
			"m_Parent->BeginModalAgain"}),
			"typed parent/child lifetime suspends, finishes, resumes and applies results in order");
		Check(controls_apply.find("if (!m_Applied") != std::string::npos
			&& Count(controls_apply, "Options.Save_Settings();") == 1
			&& controls_apply.find("m_Applied = true") != std::string::npos
			&& in_game.find("GameControls_Dialog_Continuation") != std::string::npos,
			"SETTINGS retains GameControls and applies native settings exactly once");
		Check(netshare.find("if (Session.Type == GAME_SKIRMISH)") != std::string::npos
			&& netshare.find("if (Session.Type == GAME_IPX || Session.Type == GAME_INTERNET)") != std::string::npos
			&& scenario.find("UI_Scenario_Pick_Screen_Continuation") != std::string::npos
			&& skirmish.find("Pick_Scenario_Screen_Continuation") != std::string::npos,
			"local continuations preserve legacy network paths and Skirmish child routing");
		Check(save_children.find("CHILD_OVERWRITE_CONFIRM") < save_children.find("CHILD_SAVE_WRITE")
			&& save_children.find("CHILD_DELETE_CONFIRM") < save_children.find("CHILD_DELETE_WRITE")
			&& save_children.find("UI_Message_Box_Screen_Continuation") != std::string::npos,
			"SaveBrowser queues confirmation before save/delete effects and orders modal children");
		Check(scenario_child.find("CreateRandomMap_Continuation") != std::string::npos
			&& scenario_child.find("Complete_Child_Continuation") != std::string::npos
			&& random_map_child.find("m_MapGen->Step()") != std::string::npos
			&& Count(random_map_child, "Register_Random_Map_Scenario()") == 1
			&& random_map_child.find("m_Completed") != std::string::npos,
			"ScenarioPick retains/resumes random-map children and registers accepted maps once");
		Check(skirmish_child.find("PreviousScenario = Session.Options.ScenarioIndex") != std::string::npos
			&& skirmish_child.find("Pick_Scenario_Screen_Continuation") != std::string::npos
			&& skirmish_child.find("Apply_Scenario_Pick") != std::string::npos,
			"Skirmish retains ScenarioPick across child Steps and restores its previous selection on cancel");
		Check(mapgen_cleanup.find("if (m_Cleaned) return") != std::string::npos
			&& Count(mapgen_cleanup, "Write_PCX_File") == 1
			&& Count(mapgen_cleanup, "delete RandomMapGen.MapPreview") == 1
			&& Count(mapgen_cleanup, "delete RandomMapGen.MapSeeder") == 1
			&& mapgen_ui.find("void MapGenViewClass::End_Lifetime") != std::string::npos
			&& mapgen_ui.find("Release_Preview()") != std::string::npos
			&& mapgen_ui.find("SetFinishView") != std::string::npos,
			"MapGen writes the accepted result and releases generator, seeder and preview once");
	}
}

int Test_Cooperative_Movie(void);

int main(int argc, char ** argv)
{
#ifdef OPENTS_TEST_BROWSER_MOVIES
	Failures += Test_Cooperative_Movie();
#endif
	bool const nod_smoke = argc > 1 && std::strcmp(argv[1], "--nod-smoke") == 0;
	Test_MainGameSession_Lifecycle(nod_smoke ? "Nod" : "GDI");
	if (!nod_smoke) Test_Production_Seams();
	std::printf("\n%s\n", Failures == 0 ? "PASSED" : "FAILED");
	return(Failures == 0 ? 0 : 1);
}
