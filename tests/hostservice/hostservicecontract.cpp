/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "hostruntime.hh"

#include <cstdint>
#include <deque>
#include <iostream>
#include <vector>


namespace
{
	int Failures = 0;


	void Check(char const * name, bool condition)
	{
		if (!condition) {
			std::cerr << name << " failed\n";
			Failures++;
		}
	}


	class FakeHost final : public OpenTSHost
	{
	public:
		char const * Name(void) const override { return("fake"); }
		int Run_Game(int, char **) override { return(0); }

		void Pump_Events(void) override
		{
			PumpCount++;
		}

		bool Poll_Event(OpenTSHostEvent & event) override
		{
			if (Events.empty()) {
				return(false);
			}
			event = Events.front();
			Events.pop_front();
			if (event.Type == OPENTS_HOST_EVENT_QUIT) {
				Running = false;
			}
			return(true);
		}

		bool Is_Running(void) const override { return(Running); }

		int PumpCount = 0;
		bool Running = true;
		std::deque<OpenTSHostEvent> Events;
	};


	struct Recording
	{
		std::vector<OpenTSHostEvent> Events;
		std::vector<std::int64_t> EventTimes;
		std::vector<std::int64_t> TickTimes;
		int PresentCount = 0;
		std::int64_t Now = 1000;
	};

	Recording * ActiveRecording = nullptr;


	void Deliver(OpenTSHostEvent const & event, std::int64_t now)
	{
		ActiveRecording->Events.push_back(event);
		ActiveRecording->EventTimes.push_back(now);
	}


	void Present(void)
	{
		ActiveRecording->PresentCount++;
	}


	void Tick(std::int64_t now)
	{
		ActiveRecording->TickTimes.push_back(now);
	}


	std::int64_t Now(void)
	{
		return(ActiveRecording->Now);
	}
}


int main(void)
{
	FakeHost host;
	OpenTSHostEvent key;
	key.Type = OPENTS_HOST_EVENT_KEY;
	key.Key = (unsigned short)'A';
	key.Shift = true;
	host.Events.push_back(key);

	OpenTSHostEvent mouse;
	mouse.Type = OPENTS_HOST_EVENT_MOUSE_MOVE;
	mouse.X = 17;
	mouse.Y = 23;
	host.Events.push_back(mouse);

	OpenTSHostEvent focus;
	focus.Type = OPENTS_HOST_EVENT_FOCUS;
	focus.Focused = false;
	host.Events.push_back(focus);

	OpenTSHostEvent quit;
	quit.Type = OPENTS_HOST_EVENT_QUIT;
	host.Events.push_back(quit);

	Recording recording;
	ActiveRecording = &recording;
	OpenTSHostServiceHooks hooks;
	hooks.Deliver_Event = Deliver;
	hooks.Present_If_Dirty = Present;
	hooks.Tick_ToolTips = Tick;
	hooks.Now = Now;

	Check("no host before lifetime", OpenTSHostLifetime::Current_Host() == nullptr);
	{
		OpenTSHostLifetime active(host);
		Check("active host registration", OpenTSHostLifetime::Current_Host() == &host);

		bool const alive_after_quit = OpenTS_Service_Active_Host_Once(OpenTSHostLifetime::Current_Host(), hooks);
		Check("one pump per service call", host.PumpCount == 1);
		Check("keyboard mouse focus quit order", recording.Events.size() == 4
			&& recording.Events[0].Type == OPENTS_HOST_EVENT_KEY
			&& recording.Events[1].Type == OPENTS_HOST_EVENT_MOUSE_MOVE
			&& recording.Events[2].Type == OPENTS_HOST_EVENT_FOCUS
			&& recording.Events[3].Type == OPENTS_HOST_EVENT_QUIT);
		Check("semantic fields preserved", recording.Events[0].Key == (unsigned short)'A'
			&& recording.Events[0].Shift && recording.Events[1].X == 17
			&& recording.Events[1].Y == 23 && !recording.Events[2].Focused);
		Check("quit is semantic and stops service result", !alive_after_quit);
		Check("presentation tail once", recording.PresentCount == 1);
		Check("tooltip tick once", recording.TickTimes.size() == 1
			&& recording.TickTimes[0] == recording.Now);
		Check("event timestamp deterministic", recording.EventTimes.size() == 4
			&& recording.EventTimes[0] == recording.Now
			&& recording.EventTimes[3] == recording.Now);

		bool const second_result = OpenTS_Service_Active_Host_Once(OpenTSHostLifetime::Current_Host(), hooks);
		Check("second service pumps once again", host.PumpCount == 2);
		Check("no duplicate semantic delivery", recording.Events.size() == 4);
		Check("maintenance remains once per call", recording.PresentCount == 2
			&& recording.TickTimes.size() == 2);
		Check("quit result remains deterministic", !second_result);
	}
	Check("active host cleared at lifetime end", OpenTSHostLifetime::Current_Host() == nullptr);

	int const present_before_no_host = recording.PresentCount;
	std::size_t const ticks_before_no_host = recording.TickTimes.size();
	Check("no active host is safe", !OpenTS_Service_Active_Host_Once(nullptr, hooks)
		&& recording.PresentCount == present_before_no_host
		&& recording.TickTimes.size() == ticks_before_no_host
		&& recording.Events.size() == 4);

	ActiveRecording = nullptr;
	if (Failures != 0) {
		std::cerr << Failures << " host-service checks failed\n";
		return(1);
	}

	std::cout << "All host-service checks passed\n";
	return(0);
}
