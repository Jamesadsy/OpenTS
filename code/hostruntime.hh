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


int OpenTS_Run(int argc, char ** argv, OpenTSHost & host);
