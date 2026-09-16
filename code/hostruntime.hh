/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "hostevent.hh"
#include "nativewindow.hh"

#include <cstdint>


class OpenTSHost
{
	public:
		virtual ~OpenTSHost(void) = default;

		virtual char const * Name(void) const = 0;
		virtual int Run_Game(int argc, char ** argv) = 0;

		virtual bool Create_Diagnostic_Window(int, int) { return(false); }
		virtual NativeWindow Native_Window(void) const { return(NativeWindow{NATIVE_WINDOW_DEFAULT, nullptr, nullptr}); }
		virtual bool Drawable_Size(int &, int &) const { return(false); }
		virtual int Refresh_Rate(void) const { return(0); }
		virtual bool Window_Is_Visible(void) const { return(false); }
		virtual bool Is_Focused(void) const { return(false); }
		virtual bool Is_Running(void) const { return(false); }
		virtual void Pump_Events(void) {}
		virtual bool Poll_Event(OpenTSHostEvent &) { return(false); }
		virtual void Wait_Milliseconds(unsigned int) {}
		virtual void Request_Quit(void) {}
		virtual void Destroy_Window(void) {}
};


// The host is borrowed for exactly the duration of OpenTS_Run. Common code can ask the
// runtime to service that host, but it never receives the host's native window or framework
// object. Previous is retained only so a nested test scope restores the prior process-local
// registration deterministically; it does not create another ownership model.
class OpenTSHostLifetime
{
	public:
		explicit OpenTSHostLifetime(OpenTSHost & host) noexcept : Previous(Current)
		{
			Current = &host;
		}

		~OpenTSHostLifetime(void) noexcept
		{
			Current = Previous;
		}

		OpenTSHostLifetime(OpenTSHostLifetime const &) = delete;
		OpenTSHostLifetime & operator = (OpenTSHostLifetime const &) = delete;

		static OpenTSHost * Current_Host(void) noexcept
		{
			return(Current);
		}

	private:
		inline static OpenTSHost * Current = nullptr;
		OpenTSHost * Previous;
};


#if defined(_WIN32)
extern bool GameInFocus;
#endif


inline bool OpenTS_Game_Is_Focused(void) noexcept
{
#if defined(_WIN32)
	return(GameInFocus);
#else
	OpenTSHost * host = OpenTSHostLifetime::Current_Host();
	return(host != nullptr && host->Is_Focused());
#endif
}


// This small callback shape is also the deterministic, no-owner-data seam used by the host
// service contract tests. Production wiring supplies the already accepted keyboard,
// tooltip, presentation and monotonic-clock consumers; it carries no native platform type.
struct OpenTSHostServiceHooks
{
	void (*Deliver_Event)(OpenTSHostEvent const &, std::int64_t) = nullptr;
	void (*Present_If_Dirty)(void) = nullptr;
	void (*Tick_ToolTips)(std::int64_t) = nullptr;
	std::int64_t (*Now)(void) = nullptr;
};


// Services one already-active host exactly once. Native collection remains inside host;
// this helper only expresses the platform-neutral pump -> drain -> presentation/tooltip tail
// ordering. Poll_Event removes each event from the host queue, so one service call cannot
// deliver the same queued event twice.
inline bool OpenTS_Service_Active_Host_Once(OpenTSHost * host, OpenTSHostServiceHooks const & hooks)
{
	if (host == nullptr) {
		return(false);
	}

	host->Pump_Events();
	bool quit = false;
	OpenTSHostEvent event;
	while (host->Poll_Event(event)) {
		if (event.Type == OPENTS_HOST_EVENT_QUIT) {
			quit = true;
		}
		if (hooks.Deliver_Event != nullptr) {
			std::int64_t const now = hooks.Now != nullptr ? hooks.Now() : 0;
			hooks.Deliver_Event(event, now);
		}
	}

	if (hooks.Present_If_Dirty != nullptr) {
		hooks.Present_If_Dirty();
	}
	if (hooks.Tick_ToolTips != nullptr) {
		std::int64_t const now = hooks.Now != nullptr ? hooks.Now() : 0;
		hooks.Tick_ToolTips(now);
	}

	return(!quit && host->Is_Running());
}


// Platform-neutral semantic host service requested by common legacy loops.
bool OpenTS_Host_Service(void);

// The focus-loss loop keeps its existing two timing classes and host-service ordering.
constexpr unsigned int OpenTS_Focus_Wait_Milliseconds(bool stay_until_focused) noexcept
{
	return(stay_until_focused ? 500U : 10U);
}


struct OpenTSFocusWaitHooks
{
	void (*Wait_Milliseconds)(unsigned int) = nullptr;
	bool (*Service_Host)(void) = nullptr;
};


inline void OpenTS_Focus_Wait_Step(bool stay_until_focused, OpenTSFocusWaitHooks const & hooks)
{
	hooks.Wait_Milliseconds(OpenTS_Focus_Wait_Milliseconds(stay_until_focused));
	hooks.Service_Host();
}


constexpr bool OpenTS_Audio_Focus_Gate(bool audio_available, bool focused) noexcept
{
	return(audio_available && focused);
}


constexpr bool OpenTS_Init_Game_Failure_Presentation_Requested(int result) noexcept
{
	return(result < 0);
}


void OpenTS_Host_Wait_Milliseconds(unsigned int milliseconds);


int OpenTS_Run(int argc, char ** argv, OpenTSHost & host);
