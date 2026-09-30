/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "newmenu.h"

#include "browsertelemetry.h"

#include "_pk.h"
#include "addon.h"
#include "ccfile.h"
#include "grphmenu.h"
#include "hostclock.h"
#include "init.h"
#include "loaddlg.h"
#include "mixfile.h"
#include "movie.h"
#include "vector.h"
#include "video.h"


/// <summary>
/// Fetches the one and only new menu object.
/// </summary>
NewMenuClass * Get_New_Menu(void)
{
	static NewMenuClass _menu;
	return(&_menu);
}


/// <summary>
/// Draws the title page behind the menus.
/// </summary>
void Draw_Menu_Background(void)
{
	Load_Title_Page(Get_New_Menu()->Background, true);
}


NewMenuClass::NewMenuClass(void) :
	MixFile(NULL),
	GameMode(-1),
	Background(NULL)
{
	if (CCFileClass("GMENU.MIX").Is_Available()) {
		MixFile = new MFCD("GMENU.MIX", &FastKey);
		if (CCFileClass("Loading.PCX").Is_Available()) {
			Background = new char[16];
			strcpy(Background, "Loading.PCX");
		}
	}

	if (Background == NULL) {
		Background = new char[10];
		strcpy(Background, "Title.PCX");
	}
}


NewMenuClass::~NewMenuClass(void)
{
	delete [] Background;
	delete MixFile;
}


GraphicMenu * NewMenuClass::Build_Menu(char const * section, DynamicVectorClass<int> & options)
{
	GraphicMenu * menu = Do_Graphic_Menu("NewMenu.INI", section);
	if (menu == NULL) return(NULL);

	delete Background;
	char const * bgname = menu->BackgroundName.Peek();
	if (bgname == NULL) bgname = "Title.PCX";
	Background = new char[strlen(bgname) + 1];
	strcpy(Background, bgname);

	for (int option : options) menu->Set_Item_Enabled(option, false);
	return(menu);
}


int NewMenuClass::Process_Game_Select(void)
{
	NewMenuSession session;
	if (!session.Begin()) return(NSEL_OLD_MENU);
	for (;;) {
		NewMenuStepResult const result = session.Step();
		if (result == NewMenuStepResult::Complete) break;
		if (result == NewMenuStepResult::Fault) {
			session.Finish();
			return(NSEL_OLD_MENU);
		}
		Host_Sleep(Video_Milliseconds_Until_Present());
	}
	int const result = session.Result();
	session.Finish();
	return(result);
}


NewMenuSession::NewMenuSession(void) = default;


NewMenuSession::~NewMenuSession(void)
{
	Finish();
}


bool NewMenuSession::Begin(void)
{
	if (Begun) return(false);
	Menu = Get_New_Menu();
	Begun = true;
	CurrentPhase = NewMenuPhase::Begin;
	return(true);
}


bool NewMenuSession::Begin_Page(char const * section, DynamicVectorClass<int> & options, PageKind kind)
{
	OwnedPage.reset(Menu->Build_Menu(section, options));
	Page = OwnedPage.get();
	if (Page == NULL) {
		PendingResult = NSEL_OLD_MENU;
		CurrentPhase = NewMenuPhase::LegacyFallback;
		return(true);
	}
	if (!Page->Presentation_Begin()) return(false);
	CurrentPage = kind;
	CurrentPhase = NewMenuPhase::PresentProductPage;
	return(true);
}


bool NewMenuSession::Begin_Title_Media(char const * name)
{
	if (!CCFileClass(name).Is_Available()) {
		CurrentPhase = Menu->GameMode == 0 ? NewMenuPhase::PrepareTS : NewMenuPhase::PrepareFirestorm;
		return(true);
	}
	TitleMedia = std::make_unique<TitleMovieSession>();
	if (!TitleMedia->Begin(name, THEME_NONE, false, true, false)) return(false);
	TitleWasAvailable = TitleMedia->Was_Available();
	CurrentPhase = NewMenuPhase::AwaitTitleMedia;
	return(true);
}


void NewMenuSession::Apply_Product_Choice(int item)
{
	OpenTS_Browser_Telemetry_Increment(OPENTS_TELEMETRY_PRODUCT_CHOICE_APPLIED);
	OpenTS_Browser_Telemetry_Set(OPENTS_TELEMETRY_LAST_PRODUCT_CHOICE_ITEM, item);
	OpenTS_Browser_Telemetry_Set(OPENTS_TELEMETRY_PRODUCT_CHOICE_SEQUENCE,
		OpenTS_Browser_Telemetry_Next_Event());
	if (item == GMENU_TIBSUN) {
		Menu->GameMode = 0;
		Disable_Addon(ADDON_ANY);
		Set_Required_Addon(ADDON_BASE_GAME);
	} else {
		Menu->GameMode = 1;
		Disable_Addon(ADDON_ANY);
		Enable_Addon(ADDON_FIRESTORM);
		Set_Required_Addon(ADDON_FIRESTORM);
	}
	Session.Read_Scenario_Descriptions();
}


NewMenuStepResult NewMenuSession::Fault(void)
{
	CurrentPhase = NewMenuPhase::Fault;
	return(NewMenuStepResult::Fault);
}


NewMenuStepResult NewMenuSession::Step(void)
{
	using Result = NewMenuStepResult;
	using Phase = NewMenuPhase;
	if (!Begun) return(Fault());
	if (CurrentPhase == Phase::Complete) return(Result::Complete);
	if (CurrentPhase == Phase::Fault) return(Result::Fault);

	switch (CurrentPhase) {
		case Phase::Begin:
			if (Menu == NULL) return(Fault());
			if (Menu->MixFile == NULL) {
				PendingResult = NSEL_OLD_MENU;
				CurrentPhase = Phase::LegacyFallback;
			} else {
				CurrentPhase = Phase::ChooseProduct;
			}
			return(Result::Waiting);

		case Phase::ChooseProduct:
			if (Menu->GameMode == -1) {
				if (Addon_Installed(ADDON_FIRESTORM)) {
					Disable_Addon(ADDON_ANY);
					DynamicVectorClass<int> options;
					if (!Begin_Page("MainMenu", options, PageKind::Selector)) return(Fault());
					return(Result::Waiting);
				}
				Menu->GameMode = 0;
				if (!Begin_Title_Media("TS_Title.VQA")) return(Fault());
				return(Result::Waiting);
			}
			CurrentPhase = Menu->GameMode == 0 ? Phase::PrepareTS : Phase::PrepareFirestorm;
			return(Result::Waiting);

		case Phase::AwaitTitleMedia: {
			if (TitleMedia == nullptr) return(Fault());
			TitleMovieStepResult const result = TitleMedia->Step();
			if (result == TitleMovieStepResult::Fault) return(Fault());
			if (result == TitleMovieStepResult::Waiting) return(Result::Waiting);
			TitleWasAvailable = TitleMedia->Was_Available();
			TitleCompletedNaturally = TitleMedia->Finished_Naturally();
			TitleWasSkipped = TitleMedia->Was_Skipped();
			if (TitleWasAvailable && !TitleCompletedNaturally && !TitleWasSkipped) return(Fault());
			if (!TitleMedia->Finish()) return(Fault());
			TitleMedia.reset();
			CurrentPhase = Menu->GameMode == 0 ? Phase::PrepareTS : Phase::PrepareFirestorm;
			return(Result::Waiting);
		}

		case Phase::PrepareTS: {
			Disable_Addon(ADDON_ANY);
			Set_Required_Addon(ADDON_BASE_GAME);
			DynamicVectorClass<int> options;
			if (!Addon_Installed(ADDON_ANY)) options.Add(102);
			if (!LoadOptionsClass().Files_Present()) options.Add(NSEL_LOAD_MISSION);
			options.Add(NSEL_INTERNET);
			if (!Begin_Page("TiberianSunMenu", options, PageKind::TiberianSun)) return(Fault());
			return(Result::Waiting);
		}

		case Phase::PrepareFirestorm: {
			Disable_Addon(ADDON_ANY);
			Enable_Addon(ADDON_FIRESTORM);
			Set_Required_Addon(ADDON_FIRESTORM);
			DynamicVectorClass<int> options;
			if (!LoadOptionsClass().Files_Present()) options.Add(NSEL_LOAD_MISSION);
			options.Add(NSEL_INTERNET);
			options.Add(NSEL_WDT);
			if (!Begin_Page("FirestormMenu", options, PageKind::Firestorm)) return(Fault());
			return(Result::Waiting);
		}

		case Phase::PresentProductPage:
		case Phase::AwaitGraphicAction: {
			if (Page == NULL) return(Fault());
			GraphicMenuStepResult const result = Page->Presentation_Step();
			if (result == GraphicMenuStepResult::Fault) return(Fault());
			if (result == GraphicMenuStepResult::Waiting) {
				CurrentPhase = Page->Presentation_Awaiting_Action()
					? Phase::AwaitGraphicAction : Phase::PresentProductPage;
				return(Result::Waiting);
			}
			PendingResult = Page->Presentation_Result();
			if (!Page->Presentation_Finish()) return(Fault());
			OwnedPage.reset();
			Page = NULL;
			CurrentPhase = Phase::MapResult;
			return(Result::Waiting);
		}

		case Phase::MapResult:
			if (CurrentPage == PageKind::Selector) {
				CurrentPage = PageKind::None;
				if (PendingResult == GMENU_TIBSUN || PendingResult == GMENU_FIRESTORM) {
					Apply_Product_Choice(PendingResult);
					if (!Begin_Title_Media(PendingResult == GMENU_TIBSUN
						? "TS_Title.VQA" : "FS_Title.VQA")) return(Fault());
					return(Result::Waiting);
				}
				Session.Read_Scenario_Descriptions();
				if (PendingResult == GMENU_BACK) {
					CurrentPhase = Phase::ChooseProduct;
				} else if (PendingResult == NSEL_EXIT || PendingResult == NSEL_VERSION
					|| PendingResult == NSEL_VIEW_CREDITS) {
					CurrentPhase = Phase::Complete;
				} else {
					CurrentPhase = Phase::ChooseProduct;
				}
				return(Result::Waiting);
			}

			CurrentPage = PageKind::None;
			if (PendingResult == GMENU_BACK) {
				Menu->GameMode = -1;
				CurrentPhase = Phase::ChooseProduct;
				return(Result::Waiting);
			}
#ifdef OPENTS_IOS
			if (PendingResult == NSEL_EXIT) {
				Menu->GameMode = -1;
				CurrentPhase = Phase::ChooseProduct;
				return(Result::Waiting);
			}
#endif
			if (PendingResult == NSEL_LAN || PendingResult == NSEL_SKIRMISH) {
				CurrentPhase = Phase::LaterSessionBoundary;
				return(Result::Waiting);
			}
			CurrentPhase = Phase::Complete;
			return(Result::Waiting);

		case Phase::LaterSessionBoundary:
			CurrentPhase = Phase::Complete;
			return(Result::Waiting);

		case Phase::LegacyFallback:
			CurrentPhase = Phase::Complete;
			return(Result::Waiting);

		case Phase::Complete:
			return(Result::Complete);

		case Phase::Fault:
			return(Result::Fault);

		case Phase::Idle:
			break;
	}

	return(Fault());
}


bool NewMenuSession::Finish(void)
{
	if (Finished) return(true);
	bool success = true;
	if (TitleMedia != nullptr) {
		success = TitleMedia->Finish() && success;
		TitleWasAvailable = TitleMedia->Was_Available();
		TitleCompletedNaturally = TitleMedia->Finished_Naturally();
		TitleWasSkipped = TitleMedia->Was_Skipped();
		TitleMedia.reset();
	}
	if (Page != NULL) success = Page->Presentation_Finish() && success;
	OwnedPage.reset();
	Page = NULL;
	Finished = true;
	return(success);
}


int NewMenuSession::Result(void) const { return(PendingResult); }
NewMenuPhase NewMenuSession::Phase(void) const { return(CurrentPhase); }
bool NewMenuSession::TitleMediaAvailable(void) const { return(TitleWasAvailable); }
bool NewMenuSession::TitleMediaCompletedNaturally(void) const { return(TitleCompletedNaturally); }
bool NewMenuSession::TitleMediaSkipped(void) const { return(TitleWasSkipped); }


int NewMenuSession::Browser_Menu_State(void) const
{
	if (Page == nullptr || (CurrentPhase != NewMenuPhase::PresentProductPage
		&& CurrentPhase != NewMenuPhase::AwaitGraphicAction)) return(0);
	switch (CurrentPage) {
		case PageKind::Selector: return(1);
		case PageKind::TiberianSun: return(2);
		case PageKind::Firestorm: return(3);
		case PageKind::None: return(0);
	}
	return(0);
}


int NewMenuSession::Selected_Product(void) const
{
	return(Menu != nullptr ? Menu->GameMode : -1);
}


bool NewMenuSession::Product_Menu_Ready(void) const
{
	int const state = Browser_Menu_State();
	return(state == 2 || state == 3);
}


GraphicMenu * Do_Graphic_Menu(const char * ini, const char * name);
