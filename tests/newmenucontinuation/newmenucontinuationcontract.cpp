/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include <cstdio>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <string>

#ifndef OPENTS_SOURCE_ROOT
#error OPENTS_SOURCE_ROOT must point at the OpenTS source tree.
#endif

namespace
{

int Failures = 0;

void Check(bool condition, char const * description)
{
	std::printf("%-82s %s\n", description, condition ? "ok" : "FAILED");
	if (!condition) ++Failures;
}

std::string ReadSource(char const * path)
{
	std::string const full_path = std::string(OPENTS_SOURCE_ROOT) + "/" + path;
	std::ifstream file(full_path, std::ios::binary);
	return(std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()));
}

std::string ExtractBlock(std::string const & source, char const * signature)
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

std::size_t Count(std::string const & text, char const * needle)
{
	std::size_t count = 0;
	std::size_t offset = 0;
	while ((offset = text.find(needle, offset)) != std::string::npos) {
		++count;
		offset += std::string(needle).size();
	}
	return(count);
}

void TestGraphicMenuLifecycle(void)
{
	std::string const source = ReadSource("code/grphmenu.cpp");
	std::string const begin = ExtractBlock(source, "bool GraphicMenu::Presentation_Begin(void)");
	std::string const step = ExtractBlock(source, "GraphicMenuStepResult GraphicMenu::Presentation_Step(void)");
	std::string const finish = ExtractBlock(source, "bool GraphicMenu::Presentation_Finish(void)");
	Check(begin.find("PresentationState != PresentationPhase::Idle") != std::string::npos
		&& begin.find("Release_Pointer_To_Host()") != std::string::npos
		&& begin.find("Keyboard->Clear()") != std::string::npos,
		"GraphicMenu Begin is one-shot and preserves pointer, surface, theme, and keyboard setup");
	Check(step.find("AwaitInitialAnimation") != std::string::npos
		&& step.find("Interactive") != std::string::npos
		&& step.find("AwaitSelectVQ") != std::string::npos
		&& step.find("Host_Sleep") == std::string::npos
		&& step.find("Wait_For_Anim") == std::string::npos
		&& step.find("Wait_For_Focus") == std::string::npos,
		"GraphicMenu Step retains lifecycle state without sleeping or entering legacy wait loops");
	Check(finish.find("PresentationCleanupDone") != std::string::npos
		&& finish.find("Recapture_Pointer()") != std::string::npos
		&& finish.find("Theme.Fade_Out()") != std::string::npos,
		"GraphicMenu Finish performs idempotent pointer and theme cleanup");
	Check(step.find("PresentationState == Phase::Complete") != std::string::npos
		&& step.find("Presentation_Finish()") != std::string::npos,
		"terminal GraphicMenu Step and Finish remain repeatable");
}

void TestInputAndItemAction(void)
{
	std::string const menu = ReadSource("code/grphmenu.cpp");
	std::string const image = ReadSource("code/grphmimg.cpp");
	std::string const shortcut = ReadSource("code/grphmsct.cpp");
	std::string const step = ExtractBlock(menu, "GraphicMenuStepResult GraphicMenu::Presentation_Step(void)");
	std::string const action = ExtractBlock(image, "bool GraphicMenuImageItem::Begin_Action(MSEngine * engine)");
	std::string const action_done = ExtractBlock(image, "bool GraphicMenuImageItem::Action_Complete(MSEngine * engine)");
	Check(step.find("Keyboard->Check()") != std::string::npos
		&& step.find("Keyboard->Get()") != std::string::npos
		&& step.find("PendingKey == KN_LMOUSE || PendingKey == KN_RETURN") != std::string::npos
		&& step.find("Get_Item_For_Key(PendingKey)") != std::string::npos
		&& step.find("Get_Item_Under_Mouse(mouse)") != std::string::npos,
		"mouse, Return, shortcut, hover, and no-item input follow the existing GraphicMenu lookup path");
	Check(image.find("return(Enabled && ActiveRect.Is_Point_Within(mouse))") != std::string::npos
		&& shortcut.find("if (!Enabled)") != std::string::npos,
		"disabled image and shortcut items cannot be selected");
	Check(step.find("PresentationActionStarted") != std::string::npos
		&& action.find("GraphicMenuItem::Action(engine)") != std::string::npos
		&& action.find("new MSVQAnim") != std::string::npos
		&& action_done.find("Has_Animation(SelectAnimation)") != std::string::npos,
		"select sound and SelectVQ action start once, then wait for the engine-owned child");

	struct ActionFixture { int Starts = 0; int Sound = 0; int VQ = 0; bool Finished = false; } fixture;
	if (!fixture.Finished) { ++fixture.Starts; ++fixture.Sound; ++fixture.VQ; fixture.Finished = true; }
	if (!fixture.Finished) { ++fixture.Starts; ++fixture.Sound; ++fixture.VQ; }
	Check(fixture.Starts == 1 && fixture.Sound == 1 && fixture.VQ == 1,
		"deterministic action fixture consumes one representative selection exactly once");
}

void TestTSFirestormTransitions(void)
{
	std::string const source = ReadSource("code/newmenu.cpp");
	std::string const choice = ExtractBlock(source, "void NewMenuSession::Apply_Product_Choice(int item)");
	std::string const step = ExtractBlock(source, "NewMenuStepResult NewMenuSession::Step(void)");
	Check(step.find("Addon_Installed(ADDON_FIRESTORM)") != std::string::npos
		&& step.find("Menu->GameMode = 0") != std::string::npos
		&& step.find("PrepareTS") != std::string::npos,
		"Firestorm-absent startup follows the source-defined base TS default");
	Check(choice.find("if (item == GMENU_TIBSUN)") != std::string::npos
		&& choice.find("else {") != std::string::npos
		&& choice.find("ADDON_BASE_GAME") != std::string::npos
		&& choice.find("Enable_Addon(ADDON_FIRESTORM)") != std::string::npos
		&& choice.find("Session.Read_Scenario_Descriptions()") != std::string::npos,
		"Firestorm-present selector retains both product transitions and scenario-description refresh");
	Check(step.find("if (PendingResult == GMENU_BACK)") != std::string::npos
		&& Count(step, "Menu->GameMode = -1") >= 2
		&& step.find("#ifdef OPENTS_IOS") != std::string::npos,
		"selector back, product-page back, and conditional iOS exit retain distinct semantics");
	Check(step.find("GMENU_BACK") != std::string::npos
		&& step.find("m_Impl") == std::string::npos,
		"retained product session does not rebuild or reset its singleton on resume");
}

void TestTitleMediaChild(void)
{
	std::string const movie = ReadSource("code/movie.cpp");
	std::string const movies = ReadSource("code/movies.cpp");
	std::string const skip = ReadSource("code/movieskip.cpp");
	std::string const newmenu = ReadSource("code/newmenu.cpp");
	std::string const begin = ExtractBlock(movie, "bool TitleMovieSession::Begin(");
	std::string const step = ExtractBlock(movie, "TitleMovieStepResult TitleMovieSession::Step(void)");
	std::string const finish = ExtractBlock(movie, "bool TitleMovieSession::Finish(void)");
	std::string const await_media = ExtractBlock(newmenu, "case Phase::AwaitTitleMedia:");
	Check(begin.find("CCFileClass(m_Impl->Name.c_str()).Is_Available()") != std::string::npos
		&& begin.find("Movie_Create(") != std::string::npos
		&& begin.find("Movie_Begin_Cooperative_Playback") != std::string::npos,
		"title child preserves availability checks and uses the existing Movie_Create path");
	Check(step.find("MovieSkip::Idle()") != std::string::npos
		&& step.find("Movie_Advance_Cooperative_Frame") != std::string::npos
		&& step.find("Movie_Play(") == std::string::npos
		&& step.find("Host_Sleep") == std::string::npos,
		"title Step supports explicit skip and one existing decoder frame without Movie_Play or sleep");
	Check(finish.find("Movie_End_Cooperative_Playback") != std::string::npos
		&& finish.find("Show_Mouse()") != std::string::npos
		&& finish.find("Movie_Destroy") != std::string::npos
		&& finish.find("Map.Flag_To_Redraw") != std::string::npos
		&& finish.find("Keyboard->Clear()") != std::string::npos
		&& finish.find("Playback.reset()") != std::string::npos,
		"movie cleanup releases handle, pointer, map redraw, keyboard, and MovieSkip lifetime once");
	Check(step.find("if (finished)") != std::string::npos
		&& step.find("NaturalCompletion = true") != std::string::npos
		&& await_media.find("TitleMedia->Step()") != std::string::npos
		&& await_media.find("TitleCompletedNaturally = TitleMedia->Finished_Naturally()") != std::string::npos
		&& await_media.find("TitleWasSkipped = TitleMedia->Was_Skipped()") != std::string::npos
		&& await_media.find("if (TitleWasAvailable && !TitleCompletedNaturally && !TitleWasSkipped)") != std::string::npos
		&& await_media.find("TitleMedia.reset()") != std::string::npos,
		"natural completion and source-authorized skip each resume the NewMenu parent once");
	Check(movies.find("CooperativePlayback.StartFrame") != std::string::npos
		&& movies.find("CooperativePlayback.EndFrame") != std::string::npos
		&& movies.find("CooperativePlayback.Iterations") != std::string::npos
		&& movies.find("Seek_To_Frame") != std::string::npos
		&& skip.find("Win_Set_Movie_Playing(true)") != std::string::npos
		&& skip.find("Win_Set_Movie_Playing(false)") != std::string::npos,
		"retained movie state preserves start/end, loop iterations, seeking, and global playback lifetime");

	struct MediaFixture {
		int Begins = 0;
		int Steps = 0;
		int Finishes = 0;
		int ParentResumes = 0;
		bool Available = false;
		bool Natural = false;
		bool Skipped = false;
	};
	for (int outcome : {0, 1, 2}) {
		MediaFixture fixture;
		fixture.Begins++;
		fixture.Available = outcome != 0;
		if (fixture.Available) {
			fixture.Steps++;
			fixture.Natural = outcome == 1;
			fixture.Skipped = outcome == 2;
		}
		if (!fixture.Available || fixture.Natural || fixture.Skipped) {
			fixture.Finishes++;
			fixture.ParentResumes++;
		}
		bool const resumed_once = fixture.Begins == 1 && fixture.Finishes == 1
			&& fixture.ParentResumes == 1
			&& (!fixture.Available || fixture.Steps == 1)
			&& (outcome != 1 || fixture.Natural)
			&& (outcome != 2 || fixture.Skipped);
		Check(resumed_once,
			outcome == 0 ? "synthetic missing-title fixture follows the no-play path once"
				: outcome == 1 ? "synthetic natural-completion fixture resumes once and cleans up once"
				: "synthetic present-title fixture records one explicit skip and resumes once");
	}
}

void TestResultRoutingAndBoundaries(void)
{
	std::string const init = ReadSource("code/init.cpp");
	std::string const newmenu = ReadSource("code/newmenu.cpp");
	std::string const map = ExtractBlock(init, "int New_Main_Menu_Apply_Result(int selection, bool prepare_native_multiplayer)");
	std::string const selection = ExtractBlock(init, "SelectGameStepResult SelectGameSession::Step(void)");
	std::string const newmenu_step = ExtractBlock(newmenu, "NewMenuStepResult NewMenuSession::Step(void)");
	Check(map.find("NSEL_START_NEW_GAME") != std::string::npos
		&& map.find("NSEL_LOAD_MISSION") != std::string::npos
		&& map.find("NSEL_OPTIONS") != std::string::npos
		&& map.find("NSEL_INTRO") != std::string::npos
		&& map.find("NSEL_VERSION") != std::string::npos
		&& map.find("NSEL_VIEW_CREDITS") != std::string::npos
		&& map.find("NSEL_EXIT") != std::string::npos,
		"single-player native mappings preserve campaign, load, options, intro, version, credits, and exit");
	std::size_t const cancelled = selection.find("case UICampaignSelectionOutcome::CANCELLED:");
	std::size_t const accepted = selection.find("case UICampaignSelectionOutcome::PRESENTATION_FAILURE:", cancelled);
	std::string const cancellation_path = cancelled != std::string::npos
		&& accepted != std::string::npos && accepted >= cancelled
		? selection.substr(cancelled, accepted - cancelled) : std::string();
	Check(cancelled != std::string::npos
		&& selection.find("Selection = SEL_NONE", cancelled) != std::string::npos
		&& selection.find("AtIterationStart = true", cancelled) != std::string::npos
		&& cancellation_path.find("GameMode") == std::string::npos
		&& newmenu_step.find("if (PendingResult == GMENU_BACK)") != std::string::npos,
		"campaign cancellation resumes the retained selected-product state without resetting GameMode");
	Check(selection.find("SelectGameLaterBoundary::Load") != std::string::npos
		&& selection.find("SelectGameLaterBoundary::Intro") != std::string::npos
		&& selection.find("SelectGameLaterBoundary::Credits") != std::string::npos
		&& selection.find("SelectGameLaterBoundary::Lan") != std::string::npos
		&& selection.find("SelectGameLaterBoundary::Skirmish") != std::string::npos
		&& selection.find("SelectGameLaterBoundary::Online") != std::string::npos,
		"unsupported later choices return typed boundaries before native blocking dispatch");
	Check(map.find("Session.Read_MultiPlayer_Settings()") != std::string::npos
		&& map.find("if (prepare_native_multiplayer)") != std::string::npos
		&& selection.find("Session.Read_MultiPlayer_Settings()") == std::string::npos,
		"browser LAN and Skirmish boundaries precede multiplayer settings, roster, and networking setup");
}

void TestParentSchedulingAndFallback(void)
{
	std::string const init = ReadSource("code/init.cpp");
	std::string const main_game = ReadSource("code/maingamesession.cpp");
	std::string const startup = ReadSource("code/startup.cpp");
	std::string const browser = ReadSource("code/browsercoord.cpp");
	std::string const newmenu_source = ReadSource("code/newmenu.cpp");
	std::string const selection = ExtractBlock(init, "SelectGameStepResult SelectGameSession::Step(void)");
	std::string const newmenu_child = ExtractBlock(selection, "case SelectGamePhase::NewMenuChild:");
	std::string const main_step = ExtractBlock(main_game, "MainGameStepResult MainGameSession::Step(void)");
	Check(Count(newmenu_child, "m_Impl->NewMenu->Step()") == 1
		&& newmenu_child.find("New_Main_Menu_Apply_Result") != std::string::npos
		&& newmenu_child.find("NSEL_OLD_MENU") != std::string::npos,
		"SelectGameSession owns one NewMenu child Step, applies native results once, and retains fallback");
	Check(main_step.find("m_Impl->Selection.Step()") != std::string::npos
		&& main_step.find("SelectGameStepResult::LaterBoundary") != std::string::npos,
		"MainGameSession forwards the typed selection boundary without returning generic NativeChild");
	Check(startup.find("MainGameStepResult::LaterBoundary) return(3)") != std::string::npos
		&& browser.find("case 3: return(StartupAction::Boundary)") != std::string::npos
		&& browser.find("session.LaterBoundaryCode") != std::string::npos,
		"startup and browser coordination retain a typed later-boundary result and shut down cleanly");
	Check(browser.find("SetFault(session, FaultNativeChild)") != std::string::npos
		&& browser.find("if (result == Action::Boundary)") != std::string::npos,
		"unknown native children remain faults while controlled boundaries use their own path");
	Check(newmenu_source.find("NewMenuSession::Browser_Menu_State") != std::string::npos
		&& newmenu_source.find("case PageKind::TiberianSun: return(2)") != std::string::npos
		&& newmenu_source.find("case PageKind::Firestorm: return(3)") != std::string::npos
		&& startup.find("selection->Product_Menu_Ready()") != std::string::npos
		&& startup.find("legacy_frontend_ready || selected_product_ready") != std::string::npos
		&& browser.find("OpenTS_Browser_Menu_State") != std::string::npos
		&& browser.find("OpenTS_Browser_Title_Media_Skipped") != std::string::npos,
		"native browser telemetry reports the selected product menu and title-media outcome without JS-owned semantics");
}

void TestNativeAdapterAndConfiguration(void)
{
	std::string const newmenu = ReadSource("code/newmenu.cpp");
	std::string const graphic = ReadSource("code/grphmenu.cpp");
	std::string const cmake = ReadSource("code/CMakeLists.txt");
	std::string const adapter = ExtractBlock(newmenu, "int NewMenuClass::Process_Game_Select(void)");
	std::string const presentation = ExtractBlock(graphic, "int GraphicMenu::Presentation(void)");
	Check(adapter.find("NewMenuSession session") != std::string::npos
		&& adapter.find("session.Step()") != std::string::npos
		&& adapter.find("Host_Sleep(Video_Milliseconds_Until_Present())") != std::string::npos,
		"native NewMenu wrapper drives the same retained session and sleeps only outside Step");
	Check(presentation.find("Presentation_Step()") != std::string::npos
		&& presentation.find("Host_Sleep(Video_Milliseconds_Until_Present())") != std::string::npos
		&& presentation.find("Wait_For_Anim") == std::string::npos,
		"native GraphicMenu wrapper drives the same presentation session and keeps waits outside Step");
	Check(cmake.find("ASYNCIFY") == std::string::npos && cmake.find("JSPI") == std::string::npos,
		"Emscripten build remains free of Asyncify and JSPI");
}

} // namespace


int main(void)
{
	TestGraphicMenuLifecycle();
	TestInputAndItemAction();
	TestTSFirestormTransitions();
	TestTitleMediaChild();
	TestResultRoutingAndBoundaries();
	TestParentSchedulingAndFallback();
	TestNativeAdapterAndConfiguration();
	return(Failures == 0 ? 0 : 1);
}
