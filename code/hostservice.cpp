/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "hostruntime.hh"

#if defined(_WIN32)

#include "msgloop.h"

#else

#include "_keyboar.h"
#include "_tooltip.h"
#include "cctooltip.h"
#include "keyboard.h"
#include "monotonic.h"
#include "video.h"

extern bool GameInFocus;


static void Deliver_Host_Event(OpenTSHostEvent const & event, std::int64_t now)
{
	if (event.Type == OPENTS_HOST_EVENT_FOCUS) {
		GameInFocus = event.Focused;
	}

	// These are the existing common semantic consumers. They see each host event once;
	// neither consumer collects native events or knows the host's framework object.
	if (ToolTips != nullptr) {
		ToolTips->Host_Event(event, now);
	}
	if (Keyboard != nullptr) {
		Keyboard->Put_Host_Event(event);
	}
}


static std::int64_t Host_Service_Now(void)
{
	return(Monotonic_Milliseconds());
}


static void Present_If_Dirty(void)
{
	Video_Present_If_Dirty();
}


static void Tick_ToolTips(std::int64_t now)
{
	if (ToolTips != nullptr) {
		ToolTips->Tick(now);
	}
}

#endif


bool OpenTS_Host_Service(void)
{
#if defined(_WIN32)
	// The retained Windows adapter owns the complete ordering: modeless dialogs, accelerators,
	// interception, ordinary dispatch, presentation and tooltip maintenance.
	Windows_Message_Handler();
	return(true);
#else
	OpenTSHostServiceHooks hooks;
	hooks.Deliver_Event = Deliver_Host_Event;
	hooks.Present_If_Dirty = Present_If_Dirty;
	hooks.Tick_ToolTips = Tick_ToolTips;
	hooks.Now = Host_Service_Now;
	return(OpenTS_Service_Active_Host_Once(OpenTSHostLifetime::Current_Host(), hooks));
#endif
}
