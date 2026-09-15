/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "gamecontrols.h"

#include <iostream>
#include <string>
#include <vector>


namespace {

	int Failures = 0;


	void Check(std::string const & name, bool condition)
	{
		if (!condition) {
			std::cerr << name << " failed\n";
			Failures++;
		}
	}


	class RecordingService final : public GameControlsService
	{
		public:
			std::vector<std::string> Calls;
			int DirectSpeedCount = 0;
			int EventSpeedCount = 0;
			int LastDirectSpeed = -1;
			int LastEventSpeed = -1;
			int DetailCount = 0;
			int DetailRedrawCount = 0;
			int CameoSetCount = 0;
			int CameoToggleCount = 0;
			int ActionSetCount = 0;
			int ActionPropagateCount = 0;
			int ToolTipSetCount = 0;
			int ToolTipActivateCount = 0;
			int LastScrollMethod = -1;
			bool LastAutoScroll = false;
			int DifficultyCount = 0;
			int SaveCount = 0;

			void Set_Game_Speed(int setting) override
			{
				Calls.emplace_back("speed.direct");
				DirectSpeedCount++;
				LastDirectSpeed = setting;
			}

			void Queue_Game_Speed_Event(int setting) override
			{
				Calls.emplace_back("speed.event");
				EventSpeedCount++;
				LastEventSpeed = setting;
			}

			void Set_Scroll_Rate(int) override
			{
				Calls.emplace_back("scroll");
			}

			void Set_Detail_Level(int) override
			{
				Calls.emplace_back("detail");
				DetailCount++;
			}

			void Reinit_Cell_Drawers(void) override
			{
				Calls.emplace_back("detail.redraw");
				DetailRedrawCount++;
			}

			void Set_Sidebar_Cameo_Text(bool) override
			{
				Calls.emplace_back("cameo");
				CameoSetCount++;
			}

			void Toggle_Cameo_Text(bool) override
			{
				Calls.emplace_back("cameo.toggle");
				CameoToggleCount++;
			}

			void Set_Action_Lines(bool) override
			{
				Calls.emplace_back("action");
				ActionSetCount++;
			}

			void Propagate_Action_Lines(bool) override
			{
				Calls.emplace_back("action.propagate");
				ActionPropagateCount++;
			}

			void Set_ToolTips(bool) override
			{
				Calls.emplace_back("tooltip");
				ToolTipSetCount++;
			}

			void Activate_ToolTips(bool) override
			{
				Calls.emplace_back("tooltip.activate");
				ToolTipActivateCount++;
			}

			void Set_Scroll_Method(int method) override
			{
				Calls.emplace_back("coasting");
				LastScrollMethod = method;
			}

			void Set_Auto_Scroll(bool enabled) override
			{
				Calls.emplace_back("edge");
				LastAutoScroll = enabled;
			}

			void Set_Difficulty(int) override
			{
				Calls.emplace_back("difficulty");
				DifficultyCount++;
			}

			void Save_Settings(void) override
			{
				Calls.emplace_back("save");
				SaveCount++;
			}
	};


	GameControlsState Initial_State(void)
	{
		GameControlsState state;
		state.GameSpeed = 2;
		state.ScrollRate = 3;
		state.DetailLevel = 1;
		state.Difficulty = 1;
		state.SidebarCameoText = false;
		state.ActionLines = false;
		state.ToolTips = false;
		state.Coasting = true;
		state.EdgeScrolling = false;
		return(state);
	}


	void Test_Mode_Matrix(void)
	{
		GameControlsContext frontend = GameControlsContext::For_Frontend();
		Check("frontend mode", frontend.Mode == GameControlsMode::FRONTEND);
		Check("frontend speed present", frontend.HasSpeed);
		Check("frontend difficulty present", frontend.HasDifficulty);
		Check("frontend subordinate buttons absent", !frontend.HasSound && !frontend.HasKeyboard);

		GameControlsContext local_session = GameControlsContext::For_Session(true, true);
		Check("session mode", local_session.Mode == GameControlsMode::SESSION);
		Check("session local speed policy", local_session.SpeedPolicy == GameControlsSpeedPolicy::DIRECT);
		Check("session speed present", local_session.HasSpeed);
		Check("session difficulty absent", !local_session.HasDifficulty);
		Check("session subordinate buttons present", local_session.HasSound && local_session.HasKeyboard);
		Check("session sound available", local_session.SoundAvailable);

		GameControlsContext network_session = GameControlsContext::For_Session(false, false);
		Check("network session event policy", network_session.SpeedPolicy == GameControlsSpeedPolicy::NETWORK_EVENT);
		Check("unavailable session sound", !network_session.SoundAvailable);

		GameControlsContext internet = GameControlsContext::For_Internet(true);
		Check("internet mode", internet.Mode == GameControlsMode::INTERNET);
		Check("internet speed absent", !internet.HasSpeed);
		Check("internet difficulty absent", !internet.HasDifficulty);
		Check("internet subordinate buttons present", internet.HasSound && internet.HasKeyboard);
	}


	void Test_Slider_Mappings(void)
	{
		for (int position = 0; position < GameControlsPresenter::MAX_SPEED_SETTING; position++) {
			int const setting = GameControlsPresenter::Game_Speed_From_Position(position);
			Check("seven-step speed inversion", GameControlsPresenter::Game_Speed_Position(setting) == position);
		}

		for (int position = 0; position < GameControlsPresenter::MAX_SCROLL_SETTING; position++) {
			int const setting = GameControlsPresenter::Scroll_Rate_From_Position(position);
			Check("seven-step scroll inversion", GameControlsPresenter::Scroll_Rate_Position(setting) == position);
		}
	}


	void Test_Direct_Indices(void)
	{
		RecordingService service;
		GameControlsPresenter presenter(GameControlsContext::For_Frontend(), Initial_State(), service);

		for (int position = 0; position < GameControlsPresenter::MAX_DETAIL_SETTING; position++) {
			presenter.Set_Detail_Position(position);
			Check("direct detail index", presenter.State().DetailLevel == position);
		}

		for (int position = 0; position < GameControlsPresenter::MAX_DIFFICULTY_SETTING; position++) {
			presenter.Set_Difficulty_Position(position);
			Check("direct difficulty index", presenter.State().Difficulty == position);
		}
	}


	void Test_Internet_Does_Not_Mutate_Speed(void)
	{
		RecordingService service;
		GameControlsState initial = Initial_State();
		GameControlsPresenter presenter(GameControlsContext::For_Internet(true), initial, service);
		presenter.Set_Game_Speed_Position(0);
		Check("internet staged speed is ignored", presenter.State().GameSpeed == initial.GameSpeed);
		presenter.Accept();
		Check("internet direct speed mutation absent", service.DirectSpeedCount == 0);
		Check("internet speed event absent", service.EventSpeedCount == 0);
	}


	void Test_Difficulty_Is_Frontend_Only(void)
	{
		RecordingService frontend_service;
		GameControlsPresenter frontend(GameControlsContext::For_Frontend(), Initial_State(), frontend_service);
		frontend.Set_Difficulty_Position(2);
		frontend.Accept();
		Check("frontend difficulty commits", frontend_service.DifficultyCount == 1);

		RecordingService session_service;
		GameControlsPresenter session(GameControlsContext::For_Session(true, true), Initial_State(), session_service);
		session.Set_Difficulty_Position(2);
		session.Accept();
		Check("session difficulty mutation absent", session_service.DifficultyCount == 0);
	}


	void Test_Accept_And_Cancel(void)
	{
		RecordingService accept_service;
		GameControlsPresenter accepted(GameControlsContext::For_Frontend(), Initial_State(), accept_service);
		accepted.Set_Game_Speed_Position(0);
		GameControlsResult const result = accepted.Accept();
		std::size_t const calls_after_accept = accept_service.Calls.size();
		Check("accept result", result == GameControlsResult::ACCEPTED);
		Check("accept saves once", accept_service.SaveCount == 1);
		Check("accept cannot commit twice", accepted.Accept() == GameControlsResult::ACCEPTED && accept_service.Calls.size() == calls_after_accept);

		RecordingService cancel_service;
		GameControlsPresenter cancelled(GameControlsContext::For_Frontend(), Initial_State(), cancel_service);
		cancelled.Set_Game_Speed_Position(0);
		Check("cancel result", cancelled.Cancel() == GameControlsResult::CANCELLED);
		Check("cancel commits nothing", cancel_service.Calls.empty());
		Check("cancel saves nothing", cancel_service.SaveCount == 0);
	}


	void Test_Subordinate_Transitions(void)
	{
		RecordingService sound_service;
		GameControlsPresenter sound(GameControlsContext::For_Session(true, true), Initial_State(), sound_service);
		sound.Set_Scroll_Rate_Position(0);
		Check("sound transition result", sound.Request_Sound() == GameControlsResult::SOUND);
		Check("sound transition saves once", sound_service.SaveCount == 1);
		Check("sound transition applies before save", !sound_service.Calls.empty() && sound_service.Calls.back() == "save");

		RecordingService keyboard_service;
		GameControlsPresenter keyboard(GameControlsContext::For_Session(true, true), Initial_State(), keyboard_service);
		keyboard.Set_Scroll_Rate_Position(0);
		Check("keyboard transition result", keyboard.Request_Keyboard() == GameControlsResult::KEYBOARD);
		Check("keyboard transition saves once", keyboard_service.SaveCount == 1);
		Check("keyboard transition applies before save", !keyboard_service.Calls.empty() && keyboard_service.Calls.back() == "save");
	}


	void Test_Session_Ended_Is_Cancel(void)
	{
		RecordingService service;
		GameControlsPresenter presenter(GameControlsContext::For_Session(false, true), Initial_State(), service);
		presenter.Set_Game_Speed_Position(0);
		Check("session ended result", presenter.Session_Ended() == GameControlsResult::CANCELLED);
		Check("session ended commits nothing", service.Calls.empty());
		Check("session ended saves nothing", service.SaveCount == 0);
	}


	void Test_Speed_Commit_Policies(void)
	{
		RecordingService network_service;
		GameControlsPresenter network(GameControlsContext::For_Session(false, true), Initial_State(), network_service);
		network.Set_Game_Speed_Position(0);
		network.Accept();
		Check("network speed event chosen", network_service.EventSpeedCount == 1 && network_service.DirectSpeedCount == 0);
		Check("network speed event value", network_service.LastEventSpeed == 6);

		RecordingService local_service;
		GameControlsPresenter local(GameControlsContext::For_Session(true, true), Initial_State(), local_service);
		local.Set_Game_Speed_Position(0);
		local.Accept();
		Check("normal or skirmish speed direct", local_service.DirectSpeedCount == 1 && local_service.EventSpeedCount == 0);
		Check("direct speed value", local_service.LastDirectSpeed == 6);
	}


	void Test_Detail_Cameo_Action_And_ToolTips(void)
	{
		RecordingService unchanged_detail_service;
		GameControlsPresenter unchanged_detail(GameControlsContext::For_Frontend(), Initial_State(), unchanged_detail_service);
		unchanged_detail.Set_Detail_Position(1);
		unchanged_detail.Accept();
		Check("detail redraw only on change", unchanged_detail_service.DetailCount == 0 && unchanged_detail_service.DetailRedrawCount == 0);

		RecordingService changed_detail_service;
		GameControlsPresenter changed_detail(GameControlsContext::For_Frontend(), Initial_State(), changed_detail_service);
		changed_detail.Set_Detail_Position(2);
		changed_detail.Accept();
		Check("changed detail redraws", changed_detail_service.DetailCount == 1 && changed_detail_service.DetailRedrawCount == 1);

		RecordingService unchanged_cameo_service;
		GameControlsPresenter unchanged_cameo(GameControlsContext::For_Frontend(), Initial_State(), unchanged_cameo_service);
		unchanged_cameo.Set_Sidebar_Cameo_Text(false);
		unchanged_cameo.Accept();
		Check("cameo toggle only on change", unchanged_cameo_service.CameoToggleCount == 0);

		RecordingService changed_cameo_service;
		GameControlsPresenter changed_cameo(GameControlsContext::For_Frontend(), Initial_State(), changed_cameo_service);
		changed_cameo.Set_Sidebar_Cameo_Text(true);
		changed_cameo.Accept();
		Check("changed cameo toggles", changed_cameo_service.CameoSetCount == 1 && changed_cameo_service.CameoToggleCount == 1);

		RecordingService action_service;
		GameControlsPresenter action(GameControlsContext::For_Frontend(), Initial_State(), action_service);
		action.Set_Action_Lines(true);
		action.Accept();
		Check("action-line propagation", action_service.ActionSetCount == 1 && action_service.ActionPropagateCount == 1);

		RecordingService frontend_tooltip_service;
		GameControlsPresenter frontend_tooltip(GameControlsContext::For_Frontend(), Initial_State(), frontend_tooltip_service);
		frontend_tooltip.Set_ToolTips(true);
		frontend_tooltip.Accept();
		Check("frontend tooltip activation absent", frontend_tooltip_service.ToolTipSetCount == 1 && frontend_tooltip_service.ToolTipActivateCount == 0);

		RecordingService session_tooltip_service;
		GameControlsPresenter session_tooltip(GameControlsContext::For_Session(true, true), Initial_State(), session_tooltip_service);
		session_tooltip.Set_ToolTips(true);
		session_tooltip.Accept();
		Check("live tooltip activation", session_tooltip_service.ToolTipSetCount == 1 && session_tooltip_service.ToolTipActivateCount == 1);
	}


	void Test_Coasting_And_Edge_Scrolling(void)
	{
		RecordingService coasting_service;
		GameControlsPresenter coasting(GameControlsContext::For_Frontend(), Initial_State(), coasting_service);
		coasting.Set_Coasting(true);
		coasting.Accept();
		Check("checked coasting maps to zero", coasting_service.LastScrollMethod == 0);

		RecordingService no_coasting_service;
		GameControlsPresenter no_coasting(GameControlsContext::For_Frontend(), Initial_State(), no_coasting_service);
		no_coasting.Set_Coasting(false);
		no_coasting.Accept();
		Check("unchecked coasting maps to one", no_coasting_service.LastScrollMethod == 1);

		RecordingService edge_service;
		GameControlsPresenter edge(GameControlsContext::For_Frontend(), Initial_State(), edge_service);
		edge.Set_Edge_Scrolling(true);
		edge.Accept();
		Check("edge scrolling maps directly", edge_service.LastAutoScroll);
	}


	void Test_Apply_Save_Transition_Order(void)
	{
		RecordingService service;
		GameControlsState initial = Initial_State();
		GameControlsPresenter presenter(GameControlsContext::For_Session(true, true), initial, service);
		presenter.Set_Game_Speed_Position(0);
		presenter.Set_Scroll_Rate_Position(0);
		presenter.Set_Detail_Position(2);
		presenter.Set_Sidebar_Cameo_Text(true);
		presenter.Set_Action_Lines(true);
		presenter.Set_ToolTips(true);
		presenter.Set_Coasting(false);
		presenter.Set_Edge_Scrolling(true);

		Check("keyboard transition follows apply and save", presenter.Request_Keyboard() == GameControlsResult::KEYBOARD);
		std::vector<std::string> const expected = {
			"speed.direct",
			"scroll",
			"detail",
			"detail.redraw",
			"cameo",
			"cameo.toggle",
			"action",
			"action.propagate",
			"tooltip",
			"tooltip.activate",
			"coasting",
			"edge",
			"save",
		};
		Check("deterministic apply/save order", service.Calls == expected);
		Check("transition is observable after save", presenter.Result() == GameControlsResult::KEYBOARD && service.Calls.back() == "save");
	}
}


int main(void)
{
	Test_Mode_Matrix();
	Test_Slider_Mappings();
	Test_Direct_Indices();
	Test_Internet_Does_Not_Mutate_Speed();
	Test_Difficulty_Is_Frontend_Only();
	Test_Accept_And_Cancel();
	Test_Subordinate_Transitions();
	Test_Session_Ended_Is_Cancel();
	Test_Speed_Commit_Policies();
	Test_Detail_Cameo_Action_And_ToolTips();
	Test_Coasting_And_Edge_Scrolling();
	Test_Apply_Save_Transition_Order();

	if (Failures != 0) {
		std::cerr << Failures << " game-controls semantic checks failed\n";
		return(1);
	}

	std::cout << "All game-controls semantic checks passed\n";
	return(0);
}
