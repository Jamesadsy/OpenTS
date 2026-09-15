/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "gamecontrols.h"


GameControlsContext GameControlsContext::For_Frontend(void)
{
	GameControlsContext context;
	context.Mode = GameControlsMode::FRONTEND;
	context.SpeedPolicy = GameControlsSpeedPolicy::DIRECT;
	context.HasSpeed = true;
	context.HasDifficulty = true;
	context.HasSound = false;
	context.HasKeyboard = false;
	context.SoundAvailable = false;
	return(context);
}


GameControlsContext GameControlsContext::For_Session(bool local_speed, bool sound_available)
{
	GameControlsContext context;
	context.Mode = GameControlsMode::SESSION;
	context.SpeedPolicy = local_speed ? GameControlsSpeedPolicy::DIRECT : GameControlsSpeedPolicy::NETWORK_EVENT;
	context.HasSpeed = true;
	context.HasDifficulty = false;
	context.HasSound = true;
	context.HasKeyboard = true;
	context.SoundAvailable = sound_available;
	return(context);
}


GameControlsContext GameControlsContext::For_Internet(bool sound_available)
{
	GameControlsContext context;
	context.Mode = GameControlsMode::INTERNET;
	context.SpeedPolicy = GameControlsSpeedPolicy::DISABLED;
	context.HasSpeed = false;
	context.HasDifficulty = false;
	context.HasSound = true;
	context.HasKeyboard = true;
	context.SoundAvailable = sound_available;
	return(context);
}


GameControlsPresenter::GameControlsPresenter(GameControlsContext context, GameControlsState initial, GameControlsService & service) :
	ContextValue(context),
	InitialState(initial),
	StateValue(initial),
	Service(&service)
{
}


int GameControlsPresenter::Game_Speed_From_Position(int position)
{
	return(MAX_SPEED_SETTING - 1 - position);
}


int GameControlsPresenter::Game_Speed_Position(int setting)
{
	return(MAX_SPEED_SETTING - 1 - setting);
}


int GameControlsPresenter::Scroll_Rate_From_Position(int position)
{
	return(MAX_SCROLL_SETTING - 1 - position);
}


int GameControlsPresenter::Scroll_Rate_Position(int setting)
{
	return(MAX_SCROLL_SETTING - 1 - setting);
}


bool GameControlsPresenter::Can_Request_Sound(void) const
{
	return(ContextValue.HasSound && ContextValue.SoundAvailable);
}


bool GameControlsPresenter::Can_Request_Keyboard(void) const
{
	return(ContextValue.HasKeyboard);
}


int GameControlsPresenter::Game_Speed_Position(void) const
{
	return(Game_Speed_Position(StateValue.GameSpeed));
}


int GameControlsPresenter::Scroll_Rate_Position(void) const
{
	return(Scroll_Rate_Position(StateValue.ScrollRate));
}


void GameControlsPresenter::Set_Game_Speed_Position(int position)
{
	if (ContextValue.HasSpeed) {
		StateValue.GameSpeed = Game_Speed_From_Position(position);
	}
}


void GameControlsPresenter::Set_Scroll_Rate_Position(int position)
{
	StateValue.ScrollRate = Scroll_Rate_From_Position(position);
}


void GameControlsPresenter::Set_Detail_Position(int position)
{
	StateValue.DetailLevel = position;
}


void GameControlsPresenter::Set_Difficulty_Position(int position)
{
	if (ContextValue.HasDifficulty) {
		StateValue.Difficulty = position;
	}
}


void GameControlsPresenter::Set_Sidebar_Cameo_Text(bool enabled)
{
	StateValue.SidebarCameoText = enabled;
}


void GameControlsPresenter::Set_Action_Lines(bool enabled)
{
	StateValue.ActionLines = enabled;
}


void GameControlsPresenter::Set_ToolTips(bool enabled)
{
	StateValue.ToolTips = enabled;
}


void GameControlsPresenter::Set_Coasting(bool enabled)
{
	StateValue.Coasting = enabled;
}


void GameControlsPresenter::Set_Edge_Scrolling(bool enabled)
{
	StateValue.EdgeScrolling = enabled;
}


void GameControlsPresenter::Apply(void)
{
	if (ContextValue.HasSpeed && StateValue.GameSpeed != InitialState.GameSpeed) {
		switch (ContextValue.SpeedPolicy) {
			case GameControlsSpeedPolicy::DIRECT:
				Service->Set_Game_Speed(StateValue.GameSpeed);
				break;

			case GameControlsSpeedPolicy::NETWORK_EVENT:
				Service->Queue_Game_Speed_Event(StateValue.GameSpeed);
				break;

			case GameControlsSpeedPolicy::DISABLED:
				break;
		}
	}

	Service->Set_Scroll_Rate(StateValue.ScrollRate);

	if (StateValue.DetailLevel != InitialState.DetailLevel) {
		Service->Set_Detail_Level(StateValue.DetailLevel);
		Service->Reinit_Cell_Drawers();
	}

	if (StateValue.SidebarCameoText != InitialState.SidebarCameoText) {
		Service->Set_Sidebar_Cameo_Text(StateValue.SidebarCameoText);
		Service->Toggle_Cameo_Text(StateValue.SidebarCameoText);
	}

	Service->Set_Action_Lines(StateValue.ActionLines);
	Service->Propagate_Action_Lines(StateValue.ActionLines);

	Service->Set_ToolTips(StateValue.ToolTips);
	if (ContextValue.Mode != GameControlsMode::FRONTEND) {
		Service->Activate_ToolTips(StateValue.ToolTips);
	}

	Service->Set_Scroll_Method(StateValue.Coasting ? 0 : 1);
	Service->Set_Auto_Scroll(StateValue.EdgeScrolling);

	if (ContextValue.Mode == GameControlsMode::FRONTEND && ContextValue.HasDifficulty) {
		Service->Set_Difficulty(StateValue.Difficulty);
	}
}


GameControlsResult GameControlsPresenter::Commit(GameControlsResult result)
{
	if (ResultValue != GameControlsResult::PENDING) {
		return(ResultValue);
	}

	Apply();
	Service->Save_Settings();
	ResultValue = result;
	return(ResultValue);
}


GameControlsResult GameControlsPresenter::Accept(void)
{
	return(Commit(GameControlsResult::ACCEPTED));
}


GameControlsResult GameControlsPresenter::Cancel(void)
{
	if (ResultValue == GameControlsResult::PENDING) {
		ResultValue = GameControlsResult::CANCELLED;
	}
	return(ResultValue);
}


GameControlsResult GameControlsPresenter::Session_Ended(void)
{
	return(Cancel());
}


GameControlsResult GameControlsPresenter::Request_Sound(void)
{
	if (!Can_Request_Sound()) {
		return(ResultValue);
	}
	return(Commit(GameControlsResult::SOUND));
}


GameControlsResult GameControlsPresenter::Request_Keyboard(void)
{
	if (!Can_Request_Keyboard()) {
		return(ResultValue);
	}
	return(Commit(GameControlsResult::KEYBOARD));
}
