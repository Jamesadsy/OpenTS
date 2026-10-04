/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

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

int Failures = 0;

void Check(bool condition, char const * description)
{
	std::printf("%-82s %s\n", description, condition ? "ok" : "FAILED");
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

bool InOrder(std::string const & text, std::vector<std::string> const & tokens)
{
	std::size_t offset = 0;
	for (std::string const & token : tokens) {
		offset = text.find(token, offset);
		if (offset == std::string::npos) return(false);
		offset += token.size();
	}
	return(true);
}

void TestAuthorisedCallerSet(void)
{
	struct Caller { char const * name; char const * source; char const * header; };
	Caller const callers[] = {
		{"UI_Main_Menu_Screen", "code/ui/uimainmenu.cpp", "code/ui/uimainmenu.h"},
		{"UI_Version_Screen", "code/ui/uiversion.cpp", "code/ui/uiversion.h"},
		{"UI_Main_Options_Screen", "code/ui/uimainoptions.cpp", "code/ui/uimainoptions.h"},
		{"UI_Game_Type_Screen", "code/ui/uigametype.cpp", "code/ui/uigametype.h"},
		{"UI_Campaign_Screen", "code/ui/uicampaign.cpp", "code/ui/uicampaign.h"},
		{"UI_Display_Options_Screen", "code/ui/uidisplayoptions.cpp", "code/ui/uidisplayoptions.h"},
		{"UI_Display_Confirm_Screen", "code/ui/uidisplayconfirm.cpp", "code/ui/uidisplayconfirm.h"},
		{"UI_Sound_Screen", "code/ui/uisound.cpp", "code/ui/uisound.h"},
		{"UI_Keyboard_Screen", "code/ui/uikeyboard.cpp", "code/ui/uikeyboard.h"},
		{"UI_Message_Box_Screen", "code/ui/uimessagebox.cpp", "code/ui/uimessagebox.h"},
	};

	for (Caller const & caller : callers) {
		std::string const source = ReadSource(caller.source);
		std::string const header = ReadSource(caller.header);
		std::string const wrapper = ExtractBlock(source, (std::string("UIResult ") + caller.name + "(").c_str());
		std::string const factory = std::string(caller.name) + "_Continuation";
		Check(!wrapper.empty() && wrapper.find(factory) != std::string::npos
			&& wrapper.find("UI_Run_Frontend_Continuation") != std::string::npos
			&& header.find(factory) != std::string::npos,
			(std::string(caller.name) + " has a durable continuation factory and native compatibility wrapper").c_str());
	}
}

void TestTypedLifetimeAndModalDelegation(void)
{
	std::string const helper = ReadSource("code/ui/uifrontendcontinuation.h");
	std::string const adapter = ExtractBlock(ReadSource("code/ui/uishell.cpp"), "UIResult UI_Run_Frontend_Continuation(");
	std::size_t const owned_presenter = helper.find("std::unique_ptr<Presenter> m_OwnedPresenter;");
	std::size_t const view_member = helper.find("std::unique_ptr<View> m_View;");
	Check(owned_presenter != std::string::npos && view_member != std::string::npos && owned_presenter < view_member,
		"owned presenter is declared before its typed view and therefore destroyed after the view");
	Check(helper.find("m_View(std::make_unique<View>(presenter") != std::string::npos
		&& helper.find("m_Modal") != std::string::npos
		&& helper.find("UI_Modal_Session_Begin_Mode") != std::string::npos
		&& helper.find("UI_Modal_Session_Step(m_Modal)") != std::string::npos,
		"typed continuation retains the caller view and delegates to the accepted UIModalSession");
	Check(InOrder(helper, {"if (m_Finished) return(m_LastStep)", "UI_Modal_Session_Step(m_Modal)"})
		&& InOrder(helper, {"FinishModalOnly()", "m_View->Close()", "m_View.reset()", "m_Finished = true"}),
		"terminal Steps are idempotent and Finish closes the modal before releasing the typed view");
	Check(adapter.find("continuation.Step()") != std::string::npos
		&& adapter.find("Host_Sleep(Video_Milliseconds_Until_Present())") != std::string::npos
		&& adapter.find("continuation.Finish()") != std::string::npos,
		"native blocking wrapper drives the same continuation and keeps waiting outside Step");
}

void TestVersionPendingParentChild(void)
{
	std::string const source = ReadSource("code/ui/uimainmenu.cpp");
	std::string const continuation = ExtractBlock(source, "class MainMenuContinuation");
	std::string const step = ExtractBlock(continuation, "UIModalStepResult Step(void)");
	Check(InOrder(step, {"Child->Step()", "Child->Finish()", "Child.reset()", "view->Show()", "view->Sync()", "Parent.BeginModalAgain"}),
		"VersionPending child finishes once before the parent view is shown, synced, and resumed");
	Check(InOrder(step, {"Parent.Step()", "Parent.FinishModalOnly()", "view->Hide()", "Presenter.Take_Version_Pending()", "UI_Version_Screen_Continuation"})
		&& Count(step, "Presenter.Take_Version_Pending()") == 1
		&& Count(step, "UI_Version_Screen_Continuation") == 1
		&& Count(step, "Parent.BeginModalAgain") == 1,
		"nested VersionPending flow suspends and hides the parent, consumes and runs one child, then resumes once");
	Check(step.find("child_step == UIModalStepResult::Active") != std::string::npos
		&& step.find("child_step == UIModalStepResult::Waiting") != std::string::npos
		&& step.find("return(child_step)") != std::string::npos,
		"nested child remains retained across host Steps while active or waiting");
}

void TestOwnedScreenDestructionOrder(void)
{
	std::string const helper = ReadSource("code/ui/uifrontendcontinuation.h");
	std::string const version = ReadSource("code/ui/uiversion.cpp");
	std::string const message = ReadSource("code/ui/uimessagebox.cpp");
	Check(version.find("std::unique_ptr<VersionPresenterClass> presenter") != std::string::npos
		&& version.find("std::move(presenter)") != std::string::npos
		&& helper.find("std::unique_ptr<Presenter> m_OwnedPresenter;") < helper.find("std::unique_ptr<View> m_View;"),
		"Version continuation self-owns its presenter longer than its typed view");
	Check(message.find("std::unique_ptr<MessageBoxPresenterClass> presenter") != std::string::npos
		&& message.find("std::move(presenter)") != std::string::npos
		&& message.find("MessageBoxViewClass>>(") != std::string::npos,
		"MessageBox continuation self-owns its presenter longer than its typed view");
}

void TestSelectGameStateAndTransitions(void)
{
	std::string const header = ReadSource("code/init.h");
	std::string const source = ReadSource("code/init.cpp");
	std::string const step = ExtractBlock(source, "SelectGameStepResult SelectGameSession::Step(void)");
	std::string const reset = ExtractBlock(source, "void SelectGameSession::Reset(void)");
	std::string const adapter = ExtractBlock(source, "bool Select_Game(bool fade)");
	std::string const campaign = ReadSource("code/ui/uicampaign.h");
	Check(header.find("bool GameLoaded") != std::string::npos
		&& header.find("int Selection") != std::string::npos
		&& header.find("bool Process") != std::string::npos
		&& header.find("bool Fade") != std::string::npos
		&& header.find("std::uint32_t WaitDeadlineMs") != std::string::npos
		&& header.find("SelectGamePhase Phase") != std::string::npos,
		"SelectGameSession stores GameLoaded, Selection, Process, Fade, theme deadline, and phase across Steps");
	Check(step.find("case SelectGamePhase::ChildModal") != std::string::npos
		&& InOrder(step, {"m_Impl->Child->Step()", "UIModalStepResult::Active", "UIResult const child_result", "UI_Campaign_Selection_Result", "m_Impl->Child->Finish()", "Campaign = result.Campaign", "Difficulty = result.Difficulty"}),
		"campaign child result and chosen difficulty are consumed only after a resumed terminal child Step");
	std::string const return_to_product_menu = ExtractBlock(step, "auto return_to_product_menu =");
	Check(step.find("CampaignOutcome = SelectGameCampaignOutcome::Cancelled") != std::string::npos
		&& step.find("return(return_to_product_menu())") != std::string::npos
		&& InOrder(return_to_product_menu, {"Process = true", "Selection = SEL_NONE", "Phase = SelectGamePhase::MainMenu", "AtIterationStart = true"})
		&& step.find("Selection == SEL_EXIT") != std::string::npos
		&& step.find("Phase = SelectGamePhase::Exit") != std::string::npos,
		"campaign cancel and invalid campaign return to the selected-product menu while Exit stays explicit");
	Check(step.find("UI_Campaign_Commit_Difficulty(result, Options.Difficulty)") != std::string::npos
		&& step.find("WaitDeadlineMs = (std::uint32_t)(TickCount + 5 * TIMER_SECOND)") != std::string::npos
		&& campaign.find("UI_Campaign_Selection_Result") != std::string::npos,
		"accepted campaign result and difficulty commit persist before the theme-drain continuation");
	Check(reset.find("Phase = SelectGamePhase::Reset") != std::string::npos
		&& reset.find("GameLoaded = false") != std::string::npos
		&& reset.find("Selection = SEL_NONE") != std::string::npos
		&& step.find("case SelectGamePhase::Reset") != std::string::npos
		&& step.find("Scen->MissionTimer.Stop()") != std::string::npos
		&& adapter.find("select_game.Reset();") != std::string::npos
		&& adapter.find("goto restart") == std::string::npos,
		"failed scenario start returns through an explicit native Reset that restores the original preamble");
	Check(adapter.find("select_game.Step()") != std::string::npos
		&& adapter.find("select_game.Complete_Native_Child") != std::string::npos
		&& adapter.find("Host_Sleep(Video_Milliseconds_Until_Present())") != std::string::npos
		&& adapter.find("case SEL_LOAD_GAME:") != std::string::npos
		&& adapter.find("case SEL_MULTIPLAYER_GAME:") != std::string::npos
		&& adapter.find("case SEL_OPTIONS:") != std::string::npos,
		"native Select_Game remains a blocking adapter over the session with legacy child paths intact");
	Check(step.find("Host_Sleep") == std::string::npos
		&& step.find("Play_Movie") == std::string::npos
		&& step.find("Play_Intro") == std::string::npos
		&& step.find("Main_Game") == std::string::npos
		&& step.find("for (;;)") == std::string::npos
		&& step.find("while (") == std::string::npos,
		"cooperative SelectGame Step has no Host_Sleep, unbounded wait, VQA/movie, or Main_Game path");
	Check(step.find("JavaScript") == std::string::npos
		&& step.find("EM_ASM") == std::string::npos
		&& step.find("EM_JS") == std::string::npos
		&& step.find("UIResult") != std::string::npos,
		"selection and UIResult transitions stay in native C++ without JavaScript semantics");
}

void TestConfigurationAndAcceptedSeams(void)
{
	std::string const configuration = ReadSource("CMakePresets.json") + ReadSource("code/CMakeLists.txt");
	std::string const shell = ReadSource("code/ui/uishell.cpp");
	std::string const frame = ReadSource("code/mainloopsession.cpp");
	Check(configuration.find("ASYNCIFY") == std::string::npos && configuration.find("JSPI") == std::string::npos,
		"Emscripten configuration keeps Asyncify and JSPI disabled");
	Check(shell.find("Main_Loop_Session_Begin") != std::string::npos
		&& shell.find("Main_Loop_Session_Step") != std::string::npos
		&& shell.find("Main_Loop_Session_Finish") != std::string::npos
		&& frame.find("Host_Sleep") == std::string::npos,
		"front-end modal composition continues to use the accepted 124C/124B seam");
}

} // namespace

int main(void)
{
	TestAuthorisedCallerSet();
	TestTypedLifetimeAndModalDelegation();
	TestVersionPendingParentChild();
	TestOwnedScreenDestructionOrder();
	TestSelectGameStateAndTransitions();
	TestConfigurationAndAcceptedSeams();
	return(Failures == 0 ? 0 : 1);
}
