/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// The main menu. Behavior traced out of Main_Menu and Main_Menu_Dialog_Proc in init.cpp.
//
// What the extraction fixes in place: the load button is disabled when there is no saved
// game to offer, and the test is made as the screen opens rather than once at startup; the
// keys the driver watched for beside the buttons belong to the screen, not to the window it
// was drawn in, so a typed character reaches Cheat_Key_Process through an intent and the
// version screen and the credits are choices like any other; and the version screen is a
// screen of a different kind, so it nests and the view steps aside for it, which is what
// the dialog's own ShowWindow did.
//
// docs/UI_DESIGN.md, "Screens", owns the contracts this keeps to.

#include "always.h"

#include "uimainmenu.h"

#include "uirmlview.h"
#include "uifrontendcontinuation.h"
#include "uiversion.h"

#include "voc.h"
#include "globals.h"
#include "init.h"
#include "loaddlg.h"
#include "_rules.h"
#include "rules.h"
#include "scrnsel.hh"

#include <RmlUi/Core/DataModelHandle.h>
#include <RmlUi/Core/Event.h>
#include <RmlUi/Core/Input.h>

void Version_Dialog(void);


void UIMainMenuPresenterClass::Refresh(void)
{
	CanLoad = LoadOptionsClass().Files_Present();
}


/// <summary>
/// The maintenance the menu driver ran on every pass of its own loop.
/// </summary>
void UIMainMenuPresenterClass::Service(void)
{
	Title_Screen_Restore();
}


/// <summary>
/// The selection the caller's own switch is written against.
/// </summary>
int UIMainMenuPresenterClass::Selection(void) const
{
	switch (Choice) {
		case CHOICE_CAMPAIGN: return(SEL_CAMPAIGN_GAME);
		case CHOICE_LOAD: return(SEL_LOAD_GAME);
		case CHOICE_MULTIPLAYER: return(SEL_MULTIPLAYER_GAME);
		case CHOICE_INTRO: return(SEL_INTRO);
		case CHOICE_OPTIONS: return(SEL_OPTIONS);
		case CHOICE_EXIT: return(SEL_EXIT);
		case CHOICE_CREDITS: return(SEL_VIEW_CREDITS);
		default: return(SEL_NONE);
	}
}


/// <summary>
/// Runs the screen the last choice asked for, then clears the request.
/// </summary>
void UIMainMenuPresenterClass::Run_Pending(void)
{
	if (!Take_Version_Pending()) return;
	Version_Dialog();
}


bool UIMainMenuPresenterClass::Take_Version_Pending(void)
{
	if (!VersionPending) return(false);
	VersionPending = false;
	return(true);
}


void UIMainMenuPresenterClass::Execute(UIIntent const & intent)
{
	if (intent.Action == UI_MAINMENU_VERSION) {
		VersionPending = true;
		return;
	}

	if (intent.Action == UI_MAINMENU_TYPED) {
		// A cheat word is spelled out one character at a time and only the word completing
		// it answers, which is why nothing here is a choice.
		if (Cheat_Key_Process((char)intent.Value)) {
			Sound_Effect(Rule->OptionsChanged);
			Title_Screen_Restore(true);
		}
		return;
	}

	UIResult result;
	result.Outcome = UIResult::OUTCOME_ACCEPTED;

	if (intent.Action == UI_MAINMENU_CAMPAIGN) {
		Choice = CHOICE_CAMPAIGN;
	} else if (intent.Action == UI_MAINMENU_LOAD) {
		// The permission is checked when the button is pressed rather than when it was
		// enabled, because a saved game can arrive or go while the screen is up.
		if (!LoadOptionsClass().Files_Present()) {
			return;
		}
		Choice = CHOICE_LOAD;
	} else if (intent.Action == UI_MAINMENU_MULTIPLAYER) {
		Choice = CHOICE_MULTIPLAYER;
	} else if (intent.Action == UI_MAINMENU_INTRO) {
		Choice = CHOICE_INTRO;
	} else if (intent.Action == UI_MAINMENU_OPTIONS) {
		Choice = CHOICE_OPTIONS;
	} else if (intent.Action == UI_MAINMENU_CREDITS) {
		Choice = CHOICE_CREDITS;
	} else if (intent.Action == UI_MAINMENU_EXIT) {
		Choice = CHOICE_EXIT;
		result.Outcome = UIResult::OUTCOME_CANCELLED;
	} else {
		return;
	}

	result.Value = Selection();
	Result = result;
}


//---------------------------------------------------------------------------------------
// The RmlUi view.
//---------------------------------------------------------------------------------------

/// <summary>
/// The RmlUi half of the main menu.
/// </summary>
class MainMenuViewClass : public UIRmlViewClass
{
	public:
		MainMenuViewClass(UIMainMenuPresenterClass & presenter) :
			UIRmlViewClass(presenter, "mainmenu.rml"),
			Screen(presenter)
		{
		}

		virtual void Bind(Rml::DataModelConstructor & model) override;
		virtual void Sync(void) override;

	private:
		UIMainMenuPresenterClass & Screen;

		// Was a modifier held for the key that produced the character now arriving? The
		// driver read the version and credits combinations off the queue before anything
		// else saw them, so a character they produce is not a character the player typed.
		bool Modified = false;
};


void MainMenuViewClass::Bind(Rml::DataModelConstructor & model)
{
	model.Bind("canload", &Screen.CanLoad);

	model.BindEventCallback("press",
		[this](Rml::DataModelHandle, Rml::Event &, Rml::VariantList const & arguments) {
			if (arguments.empty()) return;
			Screen.Queue(UIIntent{arguments[0].Get<Rml::String>(), "", 0});
		});

	// The keys the driver watched for beside the buttons are the screen's, so the document
	// carries them: the shell's modal scope takes every key message before the game's own
	// queue sees it, and Keyboard->Check() never fires again while a document is shown.
	model.BindEventCallback("key",
		[this](Rml::DataModelHandle, Rml::Event & event, Rml::VariantList const &) {
			int const key = event.GetParameter<int>("key_identifier", Rml::Input::KI_UNKNOWN);
			bool const ctrl = event.GetParameter<bool>("ctrl_key", false);
			bool const alt = event.GetParameter<bool>("alt_key", false);

			Modified = ctrl || alt;

			if (key == Rml::Input::KI_V && ctrl && !alt) {
				Screen.Queue(UIIntent{UI_MAINMENU_VERSION, "", 0});
			} else if (key == Rml::Input::KI_C && ctrl && alt) {
				Screen.Queue(UIIntent{UI_MAINMENU_CREDITS, "", 0});
			}
		});

	// A cheat word is spelled out, so the screen wants the character rather than the key
	// that produced it.
	model.BindEventCallback("typed",
		[this](Rml::DataModelHandle, Rml::Event & event, Rml::VariantList const &) {
			// One character follows one key, so the flag answers for that character alone.
			bool const modified = Modified;
			Modified = false;
			if (modified) return;

			Rml::String const text = event.GetParameter<Rml::String>("text", Rml::String());
			for (char const letter : text) {
				Screen.Queue(UIIntent{UI_MAINMENU_TYPED, "", (int)letter});
			}
		});
}


void MainMenuViewClass::Sync(void)
{
	if (!Model) return;

	Model.DirtyVariable("canload");
}


/// <summary>
/// Shows the main menu and waits for the player to choose.
/// </summary>
UIResult UI_Main_Menu_Screen(UIMainMenuPresenterClass & presenter)
{
	std::unique_ptr<UIFrontendContinuation> continuation =
		UI_Main_Menu_Screen_Continuation(presenter, true);
	return(UI_Run_Frontend_Continuation(*continuation));
}


namespace
{

class MainMenuContinuation final : public UIFrontendContinuation
{
	public:
		explicit MainMenuContinuation(UIMainMenuPresenterClass & presenter, bool native_blocking_adapter) :
			Parent(presenter, native_blocking_adapter, nullptr),
			Presenter(presenter),
			NativeBlockingAdapter(native_blocking_adapter)
		{
		}

		OpenTS_UIModal::UIModalStepResult Step(void) noexcept override
		{
			using OpenTS_UIModal::UIModalStepResult;
			if (Finished) return(LastStep);

			if (Child != nullptr) {
				UIModalStepResult const child_step = Child->Step();
				if (child_step == UIModalStepResult::Active
					|| child_step == UIModalStepResult::Waiting) {
					return(child_step);
				}
				if (!Child->Finish()) {
					LastStep = UIModalStepResult::Fault;
					return(LastStep);
				}
				Child.reset();
				MainMenuViewClass * const view = Parent.ViewPtr();
				if (view == nullptr) {
					LastStep = UIModalStepResult::Fault;
					return(LastStep);
				}
				view->Show();
				view->Sync();
				if (!Parent.BeginModalAgain(NativeBlockingAdapter)) {
					LastStep = UIModalStepResult::Fault;
					return(LastStep);
				}
				LastStep = UIModalStepResult::Waiting;
				return(LastStep);
			}

			LastStep = Parent.Step();
			if (LastStep != UIModalStepResult::Suspended || !Presenter.VersionPending) {
				return(LastStep);
			}

			MainMenuViewClass * const view = Parent.ViewPtr();
			if (view == nullptr || !Parent.FinishModalOnly()) {
				LastStep = UIModalStepResult::Fault;
				return(LastStep);
			}
			view->Hide();
			if (!Presenter.Take_Version_Pending()) {
				LastStep = UIModalStepResult::Fault;
				return(LastStep);
			}
			Child = UI_Version_Screen_Continuation(NativeBlockingAdapter);
			if (Child == nullptr) {
				LastStep = UIModalStepResult::Fault;
				return(LastStep);
			}
			LastStep = UIModalStepResult::Waiting;
			return(LastStep);
		}

		UIResult const & Result(void) const noexcept override
		{
			return(Parent.Result());
		}

		bool Finish(void) noexcept override
		{
			if (Finished) return(true);
			bool success = true;
			if (Child != nullptr) success = Child->Finish() && success;
			Child.reset();
			success = Parent.Finish() && success;
			Finished = true;
			return(success);
		}

	private:
		UITypedFrontendContinuation<UIMainMenuPresenterClass, MainMenuViewClass> Parent;
		UIMainMenuPresenterClass & Presenter;
		bool NativeBlockingAdapter = false;
		OpenTS_UIModal::UIModalStepResult LastStep = OpenTS_UIModal::UIModalStepResult::Active;
		std::unique_ptr<UIFrontendContinuation> Child;
		bool Finished = false;
};

} // namespace


std::unique_ptr<UIFrontendContinuation> UI_Main_Menu_Screen_Continuation(
	UIMainMenuPresenterClass & presenter,
	bool native_blocking_adapter)
{
	return(std::make_unique<MainMenuContinuation>(presenter, native_blocking_adapter));
}
