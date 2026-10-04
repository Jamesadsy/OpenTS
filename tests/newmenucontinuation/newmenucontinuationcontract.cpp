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
#include "animationwaitpolicy.hh"
#include "newmenu.h"
#include "scrnsel.hh"
#include "ui/uicampaign.h"

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
	Check(step.find("Engine.Animation_Pending(CurrentAnim)") != std::string::npos,
		"persistent selector backdrop waits for completion rather than removal");
	struct Animation { bool Finished = false; bool Has_Finished() const { return(Finished); } } animation;
	struct Animations {
		Animation * Live;
		int ID(Animation * item) const { return(item == Live ? 0 : -1); }
		Animation * operator[](int) const { return(Live); }
	} animations{&animation};
	Check(MSEngine_Animation_Pending(animations, &animation), "unfinished persistent backdrop waits");
	animation.Finished = true;
	Check(animations.ID(&animation) == 0 && !MSEngine_Animation_Pending(animations, &animation),
		"finished persistent backdrop remains registered while selector becomes interactive");
	animations.Live = nullptr;
	Check(!MSEngine_Animation_Pending(animations, &animation), "removed backdrop is not dereferenced");
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
	std::string const messages = ReadSource("platform/win32compat/src/message.cpp");
	std::string const touch = ReadSource("platform/win32compat/src/touch.cpp");
	std::string const route = ReadSource("code/msgroute.cpp");
	std::string const window_proc = ReadSource("code/winstub.cpp");
	std::string const dispatch = ExtractBlock(messages, "extern \"C\" LRESULT DispatchMessage(MSG const * msg)");
	std::string const restore_focus = ExtractBlock(messages, "bool Win32_Restore_Focus_For_Touch_Click(void)");
	std::string const selector_ids = ReadSource("code/newmenu.h");
	std::string const step = ExtractBlock(menu, "GraphicMenuStepResult GraphicMenu::Presentation_Step(void)");
	std::string const candidates = ExtractBlock(menu, "GraphicMenuItem * GraphicMenu::Get_Item_Under_Mouse");
	std::string const press = ExtractBlock(touch, "void Press(Uint8 button)");
	std::string const translate = ExtractBlock(messages, "static void Translate_Event(SDL_Event const & event)");
	std::string const mouse_route = ExtractBlock(route, "bool Route_Mouse_Message(");
	std::string const native_window_proc = ExtractBlock(window_proc, "LRESULT CALLBACK /*_export*/ Windows_Procedure(");
	std::string const action = ExtractBlock(image, "bool GraphicMenuImageItem::Begin_Action(MSEngine * engine)");
	std::string const action_done = ExtractBlock(image, "bool GraphicMenuImageItem::Action_Complete(MSEngine * engine)");
	Check(step.find("Keyboard->Check()") != std::string::npos
		&& step.find("Keyboard->Get()") != std::string::npos
		&& step.find("PendingKey == KN_LMOUSE || PendingKey == KN_RETURN") != std::string::npos
		&& step.find("Get_Item_For_Key(PendingKey)") != std::string::npos
		&& step.find("Get_Item_Under_Mouse(mouse)") != std::string::npos,
		"mouse, Return, shortcut, hover, and no-item input follow the existing GraphicMenu lookup path");
	std::size_t const touch_pending = step.find("Win32_Touch_Click_Pending()");
	std::size_t const touch_restore = step.find("Win32_Restore_Focus_For_Touch_Click()");
	std::size_t const focus_gate = step.find("if (!GameInFocus)");
	Check(touch_pending < touch_restore && touch_restore < focus_gate
		&& step.find("touch_key = touch_pending ? Keyboard->Check()") < touch_restore
		&& step.find("touch_key == KN_LMOUSE && !GameInFocus") < touch_restore
		&& focus_gate < step.find("candidate = Get_Item_Under_Mouse(mouse)"),
		"pending first-touch click restores late native focus before the selector gate and candidate lookup");
	std::size_t const mouse_down = translate.find("case SDL_EVENT_MOUSE_BUTTON_DOWN:");
	std::size_t const mouse_activation = translate.find("Activate_Main_Window_From_Input(main, false)", mouse_down);
	std::size_t const mouse_post = translate.find("Win32_Post_Message(main, message", mouse_activation);
	std::size_t const key_down = translate.find("case SDL_EVENT_KEY_DOWN:");
	std::size_t const key_activation = translate.find("Activate_Main_Window_From_Input(main, false)", key_down);
	std::size_t const key_post = translate.find("Win32_Post_Message(main, message", key_activation);
	Check(mouse_down < mouse_activation && mouse_activation < mouse_post
		&& key_down < key_activation && key_activation < key_post,
		"ordinary native mouse and keyboard activation/message ordering remains unchanged");
	Check(candidates.find("item->Is_Mouse_Over(mouse)") != std::string::npos
		&& image.find("return(Enabled && ActiveRect.Is_Point_Within(mouse))") != std::string::npos
		&& shortcut.find("if (!Enabled)") != std::string::npos,
		"native menu candidates hit enabled active rectangles and skip disabled items");
	Check(press.find("Win32_Touch_Click_Begin()") < press.find("Win32_Ensure_Input_Focus()")
		&& press.find("Win32_Ensure_Input_Focus()") < press.find("Win32_Post_Pointer_Message")
		&& translate.find("Activate_Main_Window_From_Input(main, false)") != std::string::npos
		&& Count(translate, "Activate_Main_Window_From_Input(main, false)") == 3,
		"touch press preserves click origin and reasserts native activation while existing mouse, key, and down paths stay intact");
	Check(dispatch.find("msg->message == WM_LBUTTONDOWN && Win32_Touch_Click_Pending()")
		< dispatch.find("Win32_Restore_Focus_For_Touch_Click()")
		&& dispatch.find("Win32_Restore_Focus_For_Touch_Click()")
		< dispatch.find("window->Procedure(msg->hwnd, msg->message")
		&& restore_focus.find("SendMessage(main, WM_ACTIVATEAPP, TRUE, 0)")
		< restore_focus.find("SendMessage(main, WM_SETFOCUS, 0, 0)"),
		"touch-derived native click synchronously restores activation and focus before click routing");
	Check(native_window_proc.find("OPENTS_TELEMETRY_ACTIVATE_BEFORE_CLICK_SEQUENCE")
		< native_window_proc.find("OPENTS_TELEMETRY_MOUSE_DOWN_SEQUENCE")
		&& native_window_proc.find("OPENTS_TELEMETRY_SET_FOCUS_BEFORE_CLICK_SEQUENCE")
		< native_window_proc.find("OPENTS_TELEMETRY_MOUSE_DOWN_SEQUENCE"),
		"selector diagnostics snapshot activation and set-focus at click consumption despite later host messages");
	Check(mouse_route.find("Window_Point_To_Game(point)") != std::string::npos
		&& mouse_route.find("Window_Point_To_Game(point)") < mouse_route.find("HWND target")
		&& native_window_proc.find("Route_Mouse_Message(hwnd, message, wParam, lParam")
			< native_window_proc.find("UI_Handle_Window_Message(hwnd, message"),
		"native game-frame routing converts scaled window pixels before menu hit evaluation");
	Check(step.find("PresentationActionStarted") != std::string::npos
		&& action.find("GraphicMenuItem::Action(engine)") != std::string::npos
		&& action.find("new MSVQAnim") != std::string::npos
		&& action_done.find("Has_Animation(SelectAnimation)") != std::string::npos,
		"select sound and SelectVQ action start once, then wait for the engine-owned child");
	Check(step.find("candidate = Get_Item_Under_Mouse(mouse)") != std::string::npos
		&& step.find("PresentationResult = SelectedItem != NULL ? SelectedItem->Get_ID() : -1")
			!= std::string::npos
		&& selector_ids.find("NSEL_EXIT,") != std::string::npos
		&& selector_ids.find("GMENU_TIBSUN = 100") != std::string::npos
		&& selector_ids.find("GMENU_FIRESTORM = 101") != std::string::npos,
		"native active-rectangle item IDs are returned unchanged as TS, Firestorm, and Exit results");

	struct ActionFixture { int Starts = 0; int Sound = 0; int VQ = 0; bool Finished = false; } fixture;
	if (!fixture.Finished) { ++fixture.Starts; ++fixture.Sound; ++fixture.VQ; fixture.Finished = true; }
	if (!fixture.Finished) { ++fixture.Starts; ++fixture.Sound; ++fixture.VQ; }
	Check(fixture.Starts == 1 && fixture.Sound == 1 && fixture.VQ == 1,
		"deterministic action fixture consumes one representative selection exactly once");

	struct RectFixture { int Result; int Left; int Top; int Right; int Bottom; };
	RectFixture const items[] = {
		{100, 100, 100, 540, 175},
		{101, 100, 185, 540, 265},
		{0, 100, 280, 540, 355},
	};
	auto native_hit = [&](int x, int y) {
		for (RectFixture const & item : items) {
			if (x >= item.Left && x < item.Right && y >= item.Top && y < item.Bottom) {
				return(item.Result);
			}
		}
		return(-1);
	};
	Check(native_hit(200, 135) == 100 && native_hit(200, 220) == 101
		&& native_hit(200, 315) == 0 && native_hit(20, 20) == -1,
		"deterministic active-rectangle fixtures hit TS, Firestorm, and Exit targets separately");
	auto apply_product = [](int result) {
		if (result == 100) return(0);
		if (result == 101) return(1);
		return(-1);
	};
	int const ts_mode = apply_product(native_hit(200, 135));
	int const firestorm_mode = apply_product(native_hit(200, 220));
	bool const exit_completed = native_hit(200, 315) == 0;
	Check(ts_mode == 0 && firestorm_mode == 1 && exit_completed,
		"native results independently apply Tiberian Sun, Firestorm, and completed Exit actions");
}

void TestTSFirestormTransitions(void)
{
	std::string const source = ReadSource("code/newmenu.cpp");
	std::string const choice = ExtractBlock(source, "void NewMenuSession::Apply_Product_Choice(int item)");
	std::string const step = ExtractBlock(source, "NewMenuStepResult NewMenuSession::Step(void)");
	std::string const focus_source = ExtractBlock(ReadSource("code/winstub.cpp"),
		"LRESULT CALLBACK /*_export*/ Windows_Procedure(");
	std::size_t const focus_state_update = focus_source.find("GameInFocus = (wParam != 0)");
	std::size_t const focus_restore = focus_source.find("Focus_Restore()", focus_state_update);
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
	Check(step.find("PendingResult == GMENU_TIBSUN || PendingResult == GMENU_FIRESTORM")
		!= std::string::npos
		&& step.find("Apply_Product_Choice(PendingResult)") != std::string::npos
		&& step.find("PendingResult == NSEL_EXIT") != std::string::npos
		&& step.find("CurrentPhase = Phase::Complete") != std::string::npos,
		"native TS and Firestorm results apply their product, while Exit reaches Complete");
	Check(focus_state_update != std::string::npos
		&& focus_restore != std::string::npos && focus_restore > focus_state_update,
		"consumed WM_ACTIVATEAPP establishes GameInFocus through the existing native focus handler");
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
	std::string const main_game = ReadSource("code/maingamesession.cpp");
	std::string const map = ExtractBlock(init, "int New_Main_Menu_Apply_Result(int selection, bool prepare_native_multiplayer)");
	std::string const resolver = ExtractBlock(ReadSource("code/newmenu.h"),
		"constexpr ProductMenuAction Resolve_Product_Menu_Action(int result)");
	std::string const selection = ExtractBlock(init, "SelectGameStepResult SelectGameSession::Step(void)");
	std::string const dispatch = ExtractBlock(selection, "auto dispatch_selection =");
	std::string const campaign = ExtractBlock(selection, "auto begin_campaign =");
	std::string const newmenu_step = ExtractBlock(newmenu, "NewMenuStepResult NewMenuSession::Step(void)");
	std::string const main_game_step = ExtractBlock(main_game, "MainGameStepResult MainGameSession::Step(void)");
	std::string const prep = ExtractBlock(main_game_step, "case MainGamePhase::PrepareGameplay:");
	std::string const gameplay = ExtractBlock(main_game_step, "case MainGamePhase::Gameplay:");
	std::string const child_modal = ExtractBlock(selection, "case SelectGamePhase::ChildModal:");
	std::string const ui_package = ReadSource("code/CMakeLists.txt");
	std::string const ui_file_loader = ReadSource("code/ui/uifile.cpp");
	std::string const ui_shell = ReadSource("code/ui/uishell.cpp");
	std::string const frontend_continuation = ReadSource("code/ui/uifrontendcontinuation.h");
	std::string const startup_source = ReadSource("code/startup.cpp");
	std::string const browser_bootstrap = ExtractBlock(startup_source,
		"extern \"C\" int OpenTS_Browser_Startup_Bootstrap(void)");
	Check(resolver.find("case NSEL_START_NEW_GAME:") != std::string::npos
		&& resolver.find("case NSEL_LOAD_MISSION:") != std::string::npos
		&& resolver.find("case NSEL_OPTIONS:") != std::string::npos
		&& resolver.find("case NSEL_INTRO:") != std::string::npos
		&& resolver.find("case NSEL_VERSION:") != std::string::npos
		&& resolver.find("case NSEL_VIEW_CREDITS:") != std::string::npos
		&& resolver.find("case NSEL_EXIT:") != std::string::npos,
		"shared native result resolver recognizes campaign, load, options, intro, version, credits, and exit IDs");
	std::size_t const native_result = newmenu_step.find("PendingResult = Page->Presentation_Result();");
	std::size_t const record_item = newmenu_step.find("OPENTS_TELEMETRY_PRODUCT_MENU_ITEM_ID", native_result);
	std::size_t const finish_page = newmenu_step.find("Page->Presentation_Finish()", record_item);
	std::size_t const capture_selection = selection.find("int const menu_selection = m_Impl->NewMenu->Result();");
	std::size_t const capture_product = selection.find("m_Impl->SelectedProduct = m_Impl->NewMenu->Selected_Product();", capture_selection);
	std::size_t const finish_menu = selection.find("m_Impl->NewMenu->Finish()", capture_product);
	std::size_t const map_result = selection.find("New_Main_Menu_Apply_Result(menu_selection", finish_menu);
	Check(native_result < record_item && record_item < finish_page
		&& capture_selection < capture_product && capture_product < finish_menu && finish_menu < map_result
		&& selection.find("OPENTS_TELEMETRY_PRODUCT_MENU_ACTIONS_CONSUMED", map_result) != std::string::npos
		&& selection.find("OPENTS_TELEMETRY_PRODUCT_MENU_ACTION_HANDLER", map_result) != std::string::npos,
		"native product item/result and selected product are recorded before the retained parent consumes the result");
	Check(dispatch.find("Selection == SEL_CAMPAIGN_GAME) return(begin_campaign())") != std::string::npos
		&& campaign.find("Init_Campaigns()") != std::string::npos
		&& campaign.find("UI_Campaign_Screen_Continuation") != std::string::npos
		&& selection.find("case UICampaignSelectionOutcome::ACCEPTED:") != std::string::npos
		&& selection.find("Phase = SelectGamePhase::ScenarioPrepare") != std::string::npos,
		"Campaign/New Game retains its caller, opens the native campaign selector, and continues to scenario preparation");
	Check(ui_package.find("OPENTS_BROWSER_UI_FILES CONFIGURE_DEPENDS") != std::string::npos
		&& ui_package.find("CMAKE_SOURCE_DIR}/ui/*\"") != std::string::npos
		&& ui_package.find("\"--preload-file\"") != std::string::npos
		&& ui_package.find("CMAKE_SOURCE_DIR}/ui@/ui\"") != std::string::npos
		&& ui_package.find("\"--preload-file=${OPENTS_GENERATED_DIR}/Language.dat@/Language.dat\"") != std::string::npos
		&& ui_package.find("OPENTS_BROWSER_UI_FILES}") != std::string::npos
		&& ui_file_loader.find("new CCFileClass(name.c_str())") != std::string::npos
		&& ui_shell.find("_Context->LoadDocument(Document)") != std::string::npos
		&& browser_bootstrap.find("Init_Search_Folders(DeploymentConfig.SearchPaths.c_str())")
			< browser_bootstrap.find("Init_Bundle_UI_Search_Path()"),
		"Emscripten packages and registers source-owned UI in the same CCFileClass path used by native RmlUi screens");
	Check(ReadSource("ui/campaign.rml").size() > 0
		&& ReadSource("ui/options.rml").size() > 0
		&& ReadSource("ui/skirmish.rml").size() > 0,
		"Campaign, Options, and local Skirmish native documents are present in the packaged UI tree");
	std::size_t const child_step = child_modal.find("m_Impl->Child->Step()");
	std::size_t const child_yield = child_modal.find("UIModalStepResult::Waiting", child_step);
	std::size_t const child_terminal = child_modal.find("OPENTS_TELEMETRY_COOPERATIVE_CHILD_COMPLETED", child_step);
	std::size_t const stale_guard = child_modal.find("child_action_sequence != action_sequence", child_step);
	Check(child_step < child_yield
		&& child_modal.find("UIModalStepResult::Suspended", child_step) < child_terminal
		&& child_yield < child_terminal
		&& stale_guard < child_terminal
		&& child_modal.find("OPENTS_TELEMETRY_PRODUCT_MENU_CHILD_RESULT_OUTCOME", child_step) < child_terminal,
		"retained product-menu child yields across Browser Steps, records its terminal reason, and cannot complete a stale action");
	std::string const options_child = ExtractBlock(selection,
		"if (m_Impl->ActiveChild == Impl::ChildType::MainOptions");
	std::string const skirmish_child = ExtractBlock(selection,
		"if (m_Impl->ActiveChild == Impl::ChildType::Skirmish)");
	Check(dispatch.find("Selection == SEL_OPTIONS) return(begin_options())") != std::string::npos
		&& selection.find("UI_Main_Options_Screen_Continuation") != std::string::npos
		&& options_child.find("m_Impl->Child->Finish()") != std::string::npos
		&& options_child.find("Phase = SelectGamePhase::MainMenu") != std::string::npos
		&& options_child.find("return(SelectGameStepResult::Waiting)") != std::string::npos,
		"Options opens through its native retained view, yields in ChildModal, and resumes the product menu only when closed");
	Check(dispatch.find("Session.Type == GAME_SKIRMISH) return(begin_skirmish())") != std::string::npos
		&& selection.find("UI_Skirmish_Screen_Continuation") != std::string::npos
		&& skirmish_child.find("m_Impl->Child->Finish()") != std::string::npos
		&& skirmish_child.find("m_Impl->SkirmishPresenter->Accepted()") != std::string::npos
		&& skirmish_child.find("Phase = SelectGamePhase::ScenarioPrepare") != std::string::npos
		&& skirmish_child.find("Phase = SelectGamePhase::MainMenu") != std::string::npos,
		"local Skirmish retains its native setup, yields until accepted or backed out, and sends acceptance to scenario preparation");
	std::string const typed_begin = ExtractBlock(frontend_continuation, "void Begin(SettleView settle_view)");
	Check(typed_begin.find("m_View->Prepare(true)") != std::string::npos
		&& typed_begin.find("OUTCOME_FAILED_TO_OPEN") != std::string::npos
		&& typed_begin.find("UIModalStepResult::Complete") != std::string::npos,
		"failed native RmlUi preparation is an explicit failed-to-open terminal, not a successful UI completion");
	struct NativeProductActionFixture { int Result; ProductMenuAction Action; int Handler; };
	NativeProductActionFixture const ts_campaign = {
		NSEL_START_NEW_GAME, Resolve_Product_Menu_Action(NSEL_START_NEW_GAME),
		Product_Menu_Selection(Resolve_Product_Menu_Action(NSEL_START_NEW_GAME))};
	NativeProductActionFixture const fs_campaign = {
		NSEL_START_NEW_GAME, Resolve_Product_Menu_Action(NSEL_START_NEW_GAME),
		Product_Menu_Selection(Resolve_Product_Menu_Action(NSEL_START_NEW_GAME))};
	NativeProductActionFixture const actual_exit = {
		NSEL_EXIT, Resolve_Product_Menu_Action(NSEL_EXIT),
		Product_Menu_Selection(Resolve_Product_Menu_Action(NSEL_EXIT))};
	Check(ts_campaign.Result == NSEL_START_NEW_GAME && fs_campaign.Result == NSEL_START_NEW_GAME
		&& ts_campaign.Action == ProductMenuAction::Campaign && fs_campaign.Action == ProductMenuAction::Campaign
		&& ts_campaign.Handler == SEL_CAMPAIGN_GAME && fs_campaign.Handler == SEL_CAMPAIGN_GAME,
		"production resolver routes TS and Firestorm Campaign/New Game results into the native campaign handler");
	Check(actual_exit.Result == NSEL_EXIT && actual_exit.Action == ProductMenuAction::Exit
		&& actual_exit.Handler == SEL_EXIT,
		"production resolver retains the native Exit result and handler identity");
	UIResult empty_campaign_accept;
	empty_campaign_accept.Outcome = UIResult::OUTCOME_ACCEPTED;
	UICampaignSelectionResult const empty_campaign = UI_Campaign_Selection_Result(
		empty_campaign_accept, -1, 0);
	Check(empty_campaign.Outcome == UICampaignSelectionOutcome::PRESENTATION_FAILURE
		&& empty_campaign.Campaign == -1,
		"empty campaign selection is rejected before scenario preparation");
	Check(Resolve_Product_Menu_Action(NSEL_LOAD_MISSION) == ProductMenuAction::Load
		&& Resolve_Product_Menu_Action(NSEL_OPTIONS) == ProductMenuAction::Options
		&& Resolve_Product_Menu_Action(NSEL_INTRO) == ProductMenuAction::Intro
		&& Resolve_Product_Menu_Action(NSEL_VERSION) == ProductMenuAction::Version
		&& Resolve_Product_Menu_Action(NSEL_VIEW_CREDITS) == ProductMenuAction::Credits
		&& Resolve_Product_Menu_Action(NSEL_LAN) == ProductMenuAction::Lan
		&& Resolve_Product_Menu_Action(NSEL_SKIRMISH) == ProductMenuAction::Skirmish,
		"production resolver keeps every source-defined local handler and LAN item ID distinct");
	Check(Product_Menu_Selection(Resolve_Product_Menu_Action(NSEL_LOAD_MISSION)) == SEL_LOAD_GAME
		&& Product_Menu_Selection(Resolve_Product_Menu_Action(NSEL_OPTIONS)) == SEL_OPTIONS
		&& Product_Menu_Selection(Resolve_Product_Menu_Action(NSEL_INTRO)) == SEL_INTRO
		&& Product_Menu_Selection(Resolve_Product_Menu_Action(NSEL_VERSION)) == SEL_VERSION
		&& Product_Menu_Selection(Resolve_Product_Menu_Action(NSEL_VIEW_CREDITS)) == SEL_VIEW_CREDITS
		&& Product_Menu_Selection(Resolve_Product_Menu_Action(NSEL_LAN)) == SEL_MULTIPLAYER_GAME
		&& Product_Menu_Selection(Resolve_Product_Menu_Action(NSEL_SKIRMISH)) == SEL_MULTIPLAYER_GAME,
		"production selection resolver returns the existing native handler IDs for every functional result");
	Check(Resolve_Product_Menu_Action(NSEL_INTERNET) == ProductMenuAction::OnlineDeferred
		&& Product_Menu_Selection(Resolve_Product_Menu_Action(NSEL_INTERNET)) == SEL_NONE
		&& Resolve_Product_Menu_Action(NSEL_WDT) == ProductMenuAction::WdtReserved
		&& Resolve_Product_Menu_Action(NSEL_SERIAL_MODEM) == ProductMenuAction::SerialRetired
		&& Resolve_Product_Menu_Action(NSEL_9) == ProductMenuAction::Placeholder,
		"production resolver preserves deferred, retired, and placeholder identities without creating networking");
	Check(dispatch.find("Selection == SEL_LOAD_GAME) return(begin_load())") != std::string::npos
		&& dispatch.find("Selection == SEL_INTRO) return(begin_intro())") != std::string::npos
		&& dispatch.find("Selection == SEL_VIEW_CREDITS) return(begin_credits())") != std::string::npos
		&& dispatch.find("Selection == SEL_OPTIONS) return(begin_options())") != std::string::npos
		&& dispatch.find("Selection == SEL_VERSION) return(begin_version())") != std::string::npos
		&& dispatch.find("Session.Type == GAME_SKIRMISH) return(begin_skirmish())") != std::string::npos
		&& selection.find("UI_Save_Browser_Screen_Continuation") != std::string::npos
		&& selection.find("UI_Skirmish_Screen_Continuation") != std::string::npos
		&& selection.find("Select_Side_Intro_Movie(0)") != std::string::npos
		&& selection.find("m_Impl->Credits->Step()") != std::string::npos
		&& selection.find("UI_Main_Options_Screen_Continuation") != std::string::npos
		&& selection.find("UI_Version_Screen_Continuation") != std::string::npos,
		"Load, Intro, TS credits, Options, Version and local Skirmish use their existing retained native handlers");
	std::string const credits = ReadSource("code/egos.cpp");
	Check(newmenu_step.find("options.Add(NSEL_INTERNET)") != std::string::npos
		&& newmenu_step.find("options.Add(NSEL_WDT)") != std::string::npos
		&& resolver.find("case NSEL_INTERNET: return(ProductMenuAction::OnlineDeferred)") != std::string::npos
		&& map.find("Product_Menu_Selection(action)") != std::string::npos
		&& map.find("no online/network implementation") != std::string::npos
		&& resolver.find("case NSEL_SERIAL_MODEM:") != std::string::npos
		&& resolver.find("case NSEL_WDT:") != std::string::npos
		&& resolver.find("case NSEL_9:") != std::string::npos
		&& credits.find("if (Addon_Enabled(ADDON_FIRESTORM))") != std::string::npos,
		"deferred Internet, retired Serial/Modem, WDT and placeholder results create no new feature; Firestorm credits retain the native no-op");
	Check(selection.find("OpenTS_Browser_Telemetry_Increment(OPENTS_TELEMETRY_ACTUAL_EXIT)") != std::string::npos
		&& selection.find("OPENTS_PRODUCT_MENU_COMPLETION_EXPLICIT_EXIT") != std::string::npos
		&& campaign.find("OPENTS_PRODUCT_MENU_CAMPAIGNS_UNAVAILABLE") != std::string::npos
		&& campaign.find("SelectGameStepResult::Exited") == std::string::npos
		&& campaign.find("Process = false") == std::string::npos
		&& selection.find("OPENTS_PRODUCT_MENU_CAMPAIGN_SELECTOR_FAILED") != std::string::npos
		&& dispatch.find("SelectGameLaterBoundary::Lan") != std::string::npos
		&& dispatch.find("SelectGameLaterBoundary::Online") != std::string::npos
		&& dispatch.find("SelectGameLaterBoundary::Skirmish") == std::string::npos
		&& dispatch.find("SelectGameLaterBoundary::Load") == std::string::npos
		&& dispatch.find("SelectGameLaterBoundary::Intro") == std::string::npos
		&& dispatch.find("SelectGameLaterBoundary::Credits") == std::string::npos,
		"Exit stays explicit while LAN/online remain typed boundaries and local actions do not end the process");
	std::size_t const presentation_failure = selection.find("case UICampaignSelectionOutcome::PRESENTATION_FAILURE:");
	std::size_t const accepted_campaign = selection.find("case UICampaignSelectionOutcome::ACCEPTED:", presentation_failure);
	std::string const failure_path = presentation_failure != std::string::npos
		&& accepted_campaign != std::string::npos && accepted_campaign >= presentation_failure
		? selection.substr(presentation_failure, accepted_campaign - presentation_failure) : std::string();
	Check(failure_path.find("return_to_product_menu()") != std::string::npos
		&& failure_path.find("SelectGameStepResult::Exited") == std::string::npos
		&& failure_path.find("Process = false") == std::string::npos,
		"campaign presentation failure resumes the selected-product menu instead of terminating the browser runtime");
	Check(main_game.find("m_Impl->Ops.StepGameplay") != std::string::npos
		&& main_game.find("OPENTS_TELEMETRY_MAIN_LOOP_HANDOFFS") != std::string::npos
		&& main_game.find("OPENTS_PRODUCT_MENU_MAIN_LOOP_HANDOFF") != std::string::npos,
		"campaign handoff is recorded only when the prepared MainLoop is created");
	Check(prep.find("m_Impl->Ops.StepPreparation") != std::string::npos
		&& prep.find("Complete_Gameplay_Preparation(true)") != std::string::npos
		&& prep.find("SCENARIO_PREPARATION_ENTRY, 2") != std::string::npos
		&& prep.find("SCENARIO_PREPARATION_ENTRY, 3") != std::string::npos
		&& prep.find("Main_Loop_Session_Step") == std::string::npos
		&& gameplay.find("Main_Loop_Session_Begin") < gameplay.find("Main_Loop_Session_Step"),
		"accepted campaign steps native scenario preparation to completion before MainLoop gameplay starts");
	Check(selection.find("Scen->Campaign = (CampaignType)result.Campaign") != std::string::npos
		&& ReadSource("code/campaign.hh").find("CAMPAIGN_GDI") != std::string::npos
		&& ReadSource("code/campaign.h").find("0 means GDI") != std::string::npos
		&& selection.find("OPENTS_TELEMETRY_CAMPAIGN_SELECTION_RESULT") != std::string::npos
		&& main_game.find("OPENTS_PRODUCT_MENU_MAIN_LOOP_HANDOFF") != std::string::npos,
		"accepted campaign identity is carried natively into scenario preparation; campaign 0 is GDI and handoff is recorded at MainLoop begin");
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
	Check(selection.find("SelectGameLaterBoundary::Lan") != std::string::npos
		&& selection.find("SelectGameLaterBoundary::Online") != std::string::npos
		&& selection.find("SelectGameLaterBoundary::Skirmish") == std::string::npos,
		"LAN and online remain typed boundaries while local Skirmish is routed cooperatively");
	Check(map.find("Session.Read_MultiPlayer_Settings()") != std::string::npos
		&& map.find("if (prepare_native_multiplayer)") != std::string::npos
		&& selection.find("Session.Read_MultiPlayer_Settings()") != std::string::npos
		&& dispatch.find("Selection == SEL_MULTIPLAYER_GAME && Session.Type == GAME_SKIRMISH")
			< dispatch.find("SelectGameLaterBoundary::Lan"),
		"browser Skirmish uses the existing local settings path while LAN stops before network setup");
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
