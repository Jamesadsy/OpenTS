/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "grphmenu.h"

#include "browsertelemetry.h"

#include "hostclock.h"
#include "_keyboar.h"
#include "_surface.h"
#include "ccfile.h"
#include "globals.h"
#include "grphmitm.h"
#include "ini.h"
#include "keyboard.h"
#include "msanim.h"
#include "drawhelp.h"
#include "theme.h"
#include "video.h"

GraphicMenu * _Graphic_Menu(INIClass const & ini, const char * name);
GraphicMenuItem * GM_Create_Item_From_INI(const char * name, INIClass const & ini, MSEngine & engine, Point2D & image_size);


/// <summary>
/// Creates a graphic menu described by an INI file.
/// Use this routine to build one of the shell menu pages. The file is fetched through
/// the mix file system and the named section within it describes the page. The menu is
/// only built here -- the caller must run it and dispose of it.
/// </summary>
/// <param name="ini">The name of the INI file that holds the menu descriptions.</param>
/// <param name="name">The name of the section that describes this menu.</param>
/// <returns>Returns with a pointer to the menu built, or NULL if the file is missing or
/// unreadable.</returns>
GraphicMenu * Do_Graphic_Menu(const char * ini, const char * name)
{
	CCFileClass file(ini);
	if (!file.Is_Available()) {
		return(NULL);
	}
	INIClass _ini;
	GraphicMenu * menu = _ini.Load(file) > 0 ? _Graphic_Menu(_ini, name) : NULL;
	return(menu);
}


/// <summary>
/// Creates a graphic menu from an INI section.
/// This routine builds the menu, gives it whatever backdrop animation and music theme
/// the section calls for, then asks the item factory to create each listed item in
/// turn. A missing backdrop animation is not fatal -- the menu simply has none.
/// </summary>
/// <param name="ini">The database to read the menu description from.</param>
/// <param name="name">The name of the section that describes this menu.</param>
/// <returns>Returns with a pointer to the menu built, or NULL if it could not be
/// allocated.</returns>
GraphicMenu * _Graphic_Menu(INIClass const & ini, const char * name)
{
	GraphicMenu * menu = new GraphicMenu;
	char buffer[256];

	if (menu == NULL) {
		return(NULL);
	}

	bool has_background = ini.Get_String(name, "Background", "", buffer, sizeof(buffer)) > 0;
	menu->BackgroundName.Replace_With_Extension(buffer, ".PCX", sizeof(".PCX") - 1);

	Point2D pt(0,0);

	if (has_background) {
		strncat(buffer, ".VQA", sizeof(buffer) - strlen(buffer) - 1);
		MSAnim * anim = NULL;
		if (CCFileClass(buffer).Is_Available()) {
			anim = new MSVQAnim(buffer, AlternateSurface, menu->Engine.Get_Anims(), true);
		}
		if (anim == NULL) {
			anim = new MSPCXAnim(buffer, menu->Engine.Get_Anims(), true);
		}

		menu->Set_Animation(anim);
		pt = anim->Get_Rect().TopLeft;
	}

	if (ini.Get_String(name, "Theme", "", buffer, sizeof(buffer)) > 0) {
		menu->Set_Theme_Name(buffer);
	}

	char entry[8];
	int item_max = ini.Get_Int(name, "ItemMax", 100);
	for (int i = 0; i <= item_max; i++) {
		sprintf(entry, "%d", i);
		if (ini.Get_String(name, entry, "", buffer, sizeof(buffer)) > 0) {
			GraphicMenuItem * item = GM_Create_Item_From_INI(buffer, ini, menu->Engine, pt);
			if (item != NULL) {
				menu->Add_Item(item);
			}
		}
	}

	return(menu);
}


/// <summary>
/// Creates an empty graphic menu.
/// The menu comes up with the title screen behind it and the intro music playing. The
/// INI reader supplies the real backdrop, theme, and items afterward.
/// </summary>
GraphicMenu::GraphicMenu(void) :
	Engine(),
	Items(),
	CurrentAnim(NULL)
{
	BackgroundName.Set("Title.PCX");
	ThemeName.Set("Intro");
}


/// <summary>
/// Destroys the menu along with everything on it.
/// The menu owns the items that were added to it, so they are deleted here.
/// </summary>
GraphicMenu::~GraphicMenu(void)
{
	Presentation_Finish();
	BackgroundName.Release();
	ThemeName.Release();

	for (GraphicMenuItem * item : Items) {
		delete item;
	}
}


/// <summary>
/// Enables or disables the menu items carrying an identifier.
/// The shell menu handler uses this routine to shut off the choices that do not apply
/// to the page it is about to display.
/// </summary>
/// <param name="id">The identifier of the items to change.</param>
/// <param name="enabled">Should the items be enabled?</param>
void GraphicMenu::Set_Item_Enabled(int id, bool enabled)
{
	for (GraphicMenuItem * item : Items) {
		if (item->Get_ID() == id) {
			item->Set_Enabled(enabled);
		}
	}
}


/// <summary>
/// Runs the menu until the player picks something.
/// This routine starts the menu's theme and then takes over the mouse and keyboard,
/// highlighting whichever item the player is pointing at, until an item is chosen.
/// The chosen item performs its action before control is handed back.
/// </summary>
/// <returns>Returns with the identifier of the menu item the player chose.</returns>
int GraphicMenu::Presentation(void)
{
	if (!Presentation_Begin()) return(-1);
	for (;;) {
		GraphicMenuStepResult const result = Presentation_Step();
		if (result == GraphicMenuStepResult::Complete) break;
		if (result == GraphicMenuStepResult::Fault) {
			Presentation_Finish();
			return(-1);
		}
		Host_Sleep(Video_Milliseconds_Until_Present());
	}
	Presentation_Finish();
	return(Presentation_Result());
}


bool GraphicMenu::Presentation_Begin(void)
{
	if (PresentationState != PresentationPhase::Idle) return(false);
	OpenTS_Browser_Theme_Requested(ThemeName.Peek());
	Theme.Play_Song(Theme.From_Name(ThemeName.Peek()));
	Release_Pointer_To_Host();
	HiddenSurface->Fill(0);
	AlternateSurface->Fill(0);
	Keyboard->Clear();
	Engine.Request_Rebuild();
	PresentationState = PresentationPhase::AwaitInitialAnimation;
	return(true);
}


GraphicMenuStepResult GraphicMenu::Presentation_Step(void)
{
	using StepResult = GraphicMenuStepResult;
	using Phase = PresentationPhase;
	if (PresentationState == Phase::Complete) return(StepResult::Complete);
	if (PresentationState == Phase::Fault || PresentationState == Phase::Idle) {
		PresentationState = Phase::Fault;
		return(StepResult::Fault);
	}

	switch (PresentationState) {
		case Phase::AwaitInitialAnimation:
			if (Engine.Has_Animation(CurrentAnim)) {
				Engine.Cooperative_Service_Step(true);
				return(StepResult::Waiting);
			}
			Engine.Restore_Anims(AlternateSurface->Get_Rect());
			Engine.Request_Rebuild();
			Engine.Cooperative_Service_Step(true);
			PresentationState = Phase::Interactive;
			return(StepResult::Waiting);

		case Phase::Interactive: {
			if (!PresentationMouseHidden) {
				Hide_Mouse();
				PresentationMouseHidden = true;
			}
			if (!GameInFocus) {
				Engine.Cooperative_Service_Step(false);
				if (GameInFocus && PresentationMouseHidden) {
					Show_Mouse();
					PresentationMouseHidden = false;
				}
				return(StepResult::Waiting);
			}
			Show_Mouse();
			PresentationMouseHidden = false;

			Point2D const mouse(Get_Mouse_X(), Get_Mouse_Y());
			GraphicMenuItem * candidate = NULL;
			bool chosen = false;
			if (Keyboard->Check() != KN_NONE) {
				PendingKey = Keyboard->Get();
				candidate = (PendingKey == KN_LMOUSE || PendingKey == KN_RETURN)
					? Get_Item_Under_Mouse(mouse) : Get_Item_For_Key(PendingKey);
				chosen = candidate != NULL;
			} else {
				candidate = Get_Item_Under_Mouse(mouse);
			}

			if (candidate != SelectedItem) {
				if (SelectedItem != NULL) SelectedItem->Set_Selected(false);
				SelectedItem = candidate;
				if (SelectedItem != NULL) SelectedItem->Set_Selected(true);
			}
			if (chosen) PresentationState = Phase::ItemAction;
			Engine.Cooperative_Service_Step(true);
			return(StepResult::Waiting);
		}

		case Phase::ItemAction:
			if (SelectedItem == NULL) {
				PresentationState = Phase::Finish;
				return(StepResult::Waiting);
			}
			if (!PresentationActionStarted) {
				PresentationActionStarted = true;
				PresentationActionComplete = SelectedItem->Begin_Action(&Engine);
			}
			if (PresentationActionComplete) {
				PresentationState = Phase::Finish;
				return(StepResult::Waiting);
			}
			Engine.Cooperative_Service_Step(true);
			PresentationState = Phase::AwaitSelectVQ;
			return(StepResult::Waiting);

		case Phase::AwaitSelectVQ:
			if (SelectedItem == NULL || SelectedItem->Action_Complete(&Engine)) {
				PresentationState = Phase::Finish;
				return(StepResult::Waiting);
			}
			Engine.Cooperative_Service_Step(true);
			return(StepResult::Waiting);

		case Phase::Finish:
			PresentationResult = SelectedItem != NULL ? SelectedItem->Get_ID() : -1;
			Presentation_Finish();
			PresentationState = Phase::Complete;
			return(StepResult::Complete);

		case Phase::Idle:
		case Phase::Complete:
		case Phase::Fault:
			break;
	}

	PresentationState = Phase::Fault;
	return(StepResult::Fault);
}


int GraphicMenu::Presentation_Result(void) const
{
	return(PresentationResult);
}


bool GraphicMenu::Presentation_Awaiting_Action(void) const
{
	return(PresentationState == PresentationPhase::ItemAction
		|| PresentationState == PresentationPhase::AwaitSelectVQ);
}


bool GraphicMenu::Presentation_Finish(void)
{
	if (PresentationCleanupDone) return(true);
	if (PresentationState == PresentationPhase::Idle) return(true);
	if (PresentationMouseHidden) {
		Show_Mouse();
		PresentationMouseHidden = false;
	}
	Recapture_Pointer();
	Theme.Fade_Out();
	PresentationCleanupDone = true;
	if (PresentationState != PresentationPhase::Fault) {
		PresentationState = PresentationPhase::Complete;
	}
	return(true);
}


/// <summary>
/// Sets the animation that plays behind the menu.
/// The animation is handed to the menu's animation engine, taking the place of
/// whatever backdrop was there before.
/// </summary>
void GraphicMenu::Set_Animation(MSAnim *anim)
{
	MSAnim * old = CurrentAnim;
	if (old != NULL) {
		Engine.Replace_Anim(old, anim);
	} else {
		Engine.Add_Animation(anim);
	}
	CurrentAnim = anim;
}


/// <summary>
/// Sets the music theme that plays while this menu is up.
/// </summary>
void GraphicMenu::Set_Theme_Name(const char * name)
{
	ThemeName.Replace(name);
}


/// <summary>
/// Adds an item to the menu.
/// </summary>
/// <remarks>The menu takes ownership of the item and will delete it when the menu is
/// destroyed.</remarks>
void GraphicMenu::Add_Item(GraphicMenuItem * item)
{
	Items.Add(item);
}


/// <summary>
/// Fetches the menu item lying under the mouse.
/// </summary>
/// <returns>Returns with a pointer to the item beneath the mouse, or NULL if the mouse
/// is over none of them.</returns>
GraphicMenuItem * GraphicMenu::Get_Item_Under_Mouse(Point2D const & mouse)
{
	for (GraphicMenuItem * item : Items) {
		OpenTS_Browser_Telemetry_Increment(OPENTS_TELEMETRY_MENU_CANDIDATE_CHECKS);
		bool const hit = item->Is_Mouse_Over(mouse);
		OpenTS_Browser_Telemetry_Set(OPENTS_TELEMETRY_MENU_CANDIDATE_ID, item->Get_ID());
		OpenTS_Browser_Telemetry_Set(OPENTS_TELEMETRY_MENU_ACTIVE_RECT_HIT, hit ? 1 : 0);
		if (hit) {
			OpenTS_Browser_Telemetry_Increment(OPENTS_TELEMETRY_MENU_ACTIVE_RECT_HITS);
			OpenTS_Browser_Telemetry_Set(OPENTS_TELEMETRY_MENU_HIT_SEQUENCE,
				OpenTS_Browser_Telemetry_Next_Event());
			return(item);
		}
	}
	return(NULL);
}


/// <summary>
/// Fetches the menu item bound to the specified key.
/// </summary>
/// <returns>Returns with a pointer to the item that answers to the key, or NULL if no
/// item claims it.</returns>
GraphicMenuItem * GraphicMenu::Get_Item_For_Key(KeyNumType key)
{
	for (GraphicMenuItem * item : Items) {
		if (item->Is_Input_Key(key)) {
			return(item);
		}
	}
	return(NULL);
}
