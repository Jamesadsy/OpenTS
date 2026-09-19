/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include <windows.h>

#include <SDL3/SDL.h>

#include <cstdio>
#include <cstring>
#include <vector>

#include "bgfxbackend.h"

extern "C" void * Win32Compat_Native_Window_Handle(HWND window);
void Win32_Pump_Host_Events(void);

int main(int argc, char ** argv)
{
	if (argc != 2 || std::strcmp(argv[1], "--opents-donor-host-smoke") != 0) return(64);
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		std::fprintf(stderr, "OPENTS_DONOR_HOST_PROOF sdl=init-failed error=%s\n", SDL_GetError());
		return(2);
	}
	HWND const window = CreateWindowEx(0, "OpenTSDonorHostSmoke", "OpenTS donor host smoke",
		WS_OVERLAPPEDWINDOW, 0, 0, 640, 480, NULL, NULL, NULL, NULL);
	if (window == NULL) {
		std::fprintf(stderr, "OPENTS_DONOR_HOST_PROOF window=create-failed\n");
		SDL_Quit();
		return(3);
	}
	ShowWindow(window, SW_SHOW);
	Win32_Pump_Host_Events();
	RECT drawable = {};
	void * const layer = Win32Compat_Native_Window_Handle(window);
	if (layer == NULL || !GetClientRect(window, &drawable) || drawable.right <= 0 || drawable.bottom <= 0) {
		std::fprintf(stderr, "OPENTS_DONOR_HOST_PROOF drawable=invalid\n");
		DestroyWindow(window);
		SDL_Quit();
		return(4);
	}
	NativeWindow const nativewindow{NATIVE_WINDOW_DEFAULT, nullptr, layer};
	if (!Backend_Init(nativewindow, drawable.right, drawable.bottom, BACKEND_RENDERER_AUTO, false)
		|| !Backend_Set_Frame_Size(64, 48)) {
		std::fprintf(stderr, "OPENTS_DONOR_HOST_PROOF bgfx=init-failed\n");
		Backend_Shutdown();
		DestroyWindow(window);
		SDL_Quit();
		return(5);
	}
	std::printf("OPENTS_DONOR_HOST_PROOF application_owners=1 event_owners=1 drawable=valid renderer=%s\n", Backend_Renderer_Name());
	if (std::strcmp(Backend_Renderer_Name(), "Metal") != 0) {
		std::fprintf(stderr, "OPENTS_DONOR_HOST_PROOF renderer=not-metal\n");
		Backend_Shutdown();
		DestroyWindow(window);
		SDL_Quit();
		return(6);
	}
	std::vector<unsigned short> pixels(64 * 48, 0x7bef);
	for (int frame = 0; frame < 12; frame++) {
		Win32_Pump_Host_Events();
		Backend_Present(pixels.data(), 64 * (int)sizeof(unsigned short), 0, 0,
			drawable.right, drawable.bottom, BACKEND_SCALE_NEAREST, true);
		Backend_End_Frame();
	}
	std::printf("OPENTS_DONOR_HOST_PROOF event_pump=progressed frame_submission=12 shutdown=clean\n");
	Backend_Shutdown();
	DestroyWindow(window);
	SDL_Quit();
	return(0);
}
