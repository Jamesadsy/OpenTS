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

#include "bgfxbackend.h"
#include "keyboard.h"

#include <cstdio>
#include <cstring>
#include <vector>


static bool Host_Smoke_Requested(int argc, char ** argv)
{
	for (int index = 1; index < argc; index++) {
		if (strcmp(argv[index], "--opents-host-smoke") == 0) {
			return(true);
		}
	}
	return(false);
}


static bool Verify_Input_Adapter(WWKeyboardClass & keyboard)
{
	OpenTSHostEvent keydown;
	keydown.Type = OPENTS_HOST_EVENT_KEY;
	keydown.Key = VK_A;
	keydown.Shift = true;
	if (!keyboard.Put_Host_Event(keydown) || !keyboard.Down(VK_A)
		|| keyboard.Get() != (VK_A | WWKEY_SHIFT_BIT)) {
		return(false);
	}

	OpenTSHostEvent keyup = keydown;
	keyup.Release = true;
	if (!keyboard.Put_Host_Event(keyup) || keyboard.Down(VK_A)
		|| keyboard.Get() != (VK_A | WWKEY_SHIFT_BIT | WWKEY_RLS_BIT)) {
		return(false);
	}

	OpenTSHostEvent mousedown;
	mousedown.Type = OPENTS_HOST_EVENT_MOUSE_BUTTON;
	mousedown.Key = VK_LBUTTON;
	mousedown.X = 17;
	mousedown.Y = 23;
	if (!keyboard.Put_Host_Event(mousedown) || keyboard.Get() != VK_LBUTTON
		|| keyboard.MouseQX != 17 || keyboard.MouseQY != 23) {
		return(false);
	}

	OpenTSHostEvent mouseup = mousedown;
	mouseup.Release = true;
	return(keyboard.Put_Host_Event(mouseup)
		&& keyboard.Get() == (VK_LBUTTON | WWKEY_RLS_BIT));
}


static int Run_Host_Smoke(OpenTSHost & host)
{
	constexpr int windowwidth = 640;
	constexpr int windowheight = 480;
	constexpr int framewidth = 64;
	constexpr int frameheight = 48;
	constexpr int framecount = 12;

	printf("OPENTS_HOST_PROOF process=started host=%s\n", host.Name());
	if (!host.Create_Diagnostic_Window(windowwidth, windowheight)) {
		fprintf(stderr, "OPENTS_HOST_PROOF native_window=failed\n");
		return(2);
	}

	int drawablewidth = 0;
	int drawableheight = 0;
	NativeWindow const window = host.Native_Window();
	if (window.Handle == nullptr || !host.Drawable_Size(drawablewidth, drawableheight)
		|| drawablewidth <= 0 || drawableheight <= 0) {
		fprintf(stderr, "OPENTS_HOST_PROOF native_window=invalid\n");
		host.Destroy_Window();
		return(3);
	}

	printf("OPENTS_HOST_PROOF native_window=ready type=%d drawable=%dx%d refresh_hz=%d visible=%d focused=%d\n",
		(int)window.Type, drawablewidth, drawableheight, host.Refresh_Rate(), host.Window_Is_Visible(), host.Is_Focused());

	if (!Backend_Init(window, drawablewidth, drawableheight, BACKEND_RENDERER_AUTO, false)) {
		fprintf(stderr, "OPENTS_HOST_PROOF bgfx=failed\n");
		host.Destroy_Window();
		return(4);
	}
	if (!Backend_Set_Frame_Size(framewidth, frameheight)) {
		fprintf(stderr, "OPENTS_HOST_PROOF frame_texture=failed\n");
		Backend_Shutdown();
		host.Destroy_Window();
		return(5);
	}

	printf("OPENTS_HOST_PROOF bgfx=initialized renderer=%s\n", Backend_Renderer_Name());

	std::vector<unsigned short> pixels(framewidth * frameheight);
	for (int y = 0; y < frameheight; y++) {
		for (int x = 0; x < framewidth; x++) {
			unsigned short const red = (unsigned short)(x * 31 / (framewidth - 1));
			unsigned short const green = (unsigned short)(y * 63 / (frameheight - 1));
			unsigned short const blue = (unsigned short)((x + y) * 31 / (framewidth + frameheight - 2));
			pixels[y * framewidth + x] = (unsigned short)((red << 11) | (green << 5) | blue);
		}
	}

	WWKeyboardClass keyboard;
	if (!Verify_Input_Adapter(keyboard)) {
		fprintf(stderr, "OPENTS_HOST_PROOF input_adapter=failed\n");
		Backend_Shutdown();
		host.Destroy_Window();
		return(6);
	}
	printf("OPENTS_HOST_PROOF input_adapter=ready keyboard=1 mouse=1\n");

	int inputevents = 0;
	int focusevents = 0;
	int frames = 0;
	for (; frames < framecount && host.Is_Running(); frames++) {
		host.Pump_Events();
		OpenTSHostEvent event;
		while (host.Poll_Event(event)) {
			if (event.Type == OPENTS_HOST_EVENT_FOCUS) {
				focusevents++;
			}
			if (keyboard.Put_Host_Event(event)) {
				inputevents++;
			}
		}

		if (host.Drawable_Size(drawablewidth, drawableheight)) {
			Backend_On_Resize(drawablewidth, drawableheight);
		}
		Backend_Present(pixels.data(), framewidth * (int)sizeof(unsigned short), 0, 0,
			drawablewidth, drawableheight, BACKEND_SCALE_NEAREST);
		host.Wait_Milliseconds(16);
	}

	printf("OPENTS_HOST_PROOF event_pump=ready input_events=%d focus_events=%d focused=%d\n",
		inputevents, focusevents, host.Is_Focused());
	printf("OPENTS_HOST_PROOF frame_submission=ready frames=%u visible=%d\n",
		Backend_Submitted_Frame_Count(), host.Window_Is_Visible());

	host.Request_Quit();
	host.Pump_Events();
	Backend_Shutdown();
	host.Destroy_Window();
	printf("OPENTS_HOST_PROOF shutdown=clean\n");
	fflush(stdout);

	return(frames == framecount ? 0 : 7);
}


int OpenTS_Run(int argc, char ** argv, OpenTSHost & host)
{
	OpenTSHostLifetime active_host(host);

	if (Host_Smoke_Requested(argc, argv)) {
		return(Run_Host_Smoke(host));
	}
	return(host.Run_Game(argc, argv));
}
