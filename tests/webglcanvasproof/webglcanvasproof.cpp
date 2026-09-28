#include "bgfxbackend.h"
#include "nativewindow.hh"
#include "win32compat.h"

#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>
#include <emscripten/emscripten.h>

namespace {

HWND _Window = nullptr;
uint32_t _PresentCount = 0;
int _InitialWidth = 0;
int _InitialHeight = 0;
int _ResizedWidth = 0;
int _ResizedHeight = 0;
bool _Ready = false;

bool Drawable_Size(int & width, int & height)
{
	RECT client;
	if (_Window == nullptr || !GetClientRect(_Window, &client)) {
		return(false);
	}

	width = client.right - client.left;
	height = client.bottom - client.top;
	return(width > 0 && height > 0);
}

}


int main(void)
{
	return(0);
}


extern "C" EMSCRIPTEN_KEEPALIVE int VV124F_Begin(void)
{
	if (_Ready) {
		return(1);
	}

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		return(0);
	}

	_Window = CreateWindowEx(0, "VV124FWebGLCanvasProof", "VV124F WebGL Canvas Proof",
		WS_POPUP, 0, 0, 640, 480, nullptr, nullptr, nullptr, nullptr);
	if (_Window == nullptr) {
		SDL_Quit();
		return(0);
	}

	ShowWindow(_Window, SW_SHOW);
	if (!Drawable_Size(_InitialWidth, _InitialHeight)) {
		DestroyWindow(_Window);
		_Window = nullptr;
		SDL_Quit();
		return(0);
	}

	NativeWindow const nativewindow = {
		NATIVE_WINDOW_DEFAULT,
		nullptr,
		Win32Compat_Native_Window_Handle(_Window),
	};
	if (nativewindow.Handle == nullptr
		|| !Backend_Init(nativewindow, _InitialWidth, _InitialHeight, BACKEND_RENDERER_AUTO, false)) {
		DestroyWindow(_Window);
		_Window = nullptr;
		SDL_Quit();
		return(0);
	}

	bgfx::setViewName(0, "124F clear/present proof");
	_Ready = true;
	return(1);
}


extern "C" EMSCRIPTEN_KEEPALIVE int VV124F_Frame(void)
{
	if (!_Ready) {
		return(0);
	}

	int width = 0;
	int height = 0;
	if (!Drawable_Size(width, height)) {
		return(0);
	}

	bgfx::setViewRect(0, 0, 0, (uint16_t)width, (uint16_t)height);
	bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x263b5aff, 1.0f, 0);
	bgfx::touch(0);
	Backend_End_Frame();
	++_PresentCount;
	return(1);
}


extern "C" EMSCRIPTEN_KEEPALIVE int VV124F_Resize(int width, int height)
{
	if (!_Ready || width <= 0 || height <= 0) {
		return(0);
	}

	Win32Window * const window = Win32_Lookup(_Window);
	if (window == nullptr || window->Handle == nullptr) {
		return(0);
	}

	SDL_SetWindowSize(window->Handle, width, height);
	SDL_PumpEvents();
	if (!Drawable_Size(_ResizedWidth, _ResizedHeight)) {
		return(0);
	}

	Backend_On_Resize(_ResizedWidth, _ResizedHeight);
	return(_ResizedWidth == width && _ResizedHeight == height ? 1 : 0);
}


extern "C" EMSCRIPTEN_KEEPALIVE uint32_t VV124F_Present_Count(void)
{
	return(_PresentCount);
}


extern "C" EMSCRIPTEN_KEEPALIVE int VV124F_Initial_Width(void)
{
	return(_InitialWidth);
}


extern "C" EMSCRIPTEN_KEEPALIVE int VV124F_Initial_Height(void)
{
	return(_InitialHeight);
}


extern "C" EMSCRIPTEN_KEEPALIVE int VV124F_Resized_Width(void)
{
	return(_ResizedWidth);
}


extern "C" EMSCRIPTEN_KEEPALIVE int VV124F_Resized_Height(void)
{
	return(_ResizedHeight);
}


extern "C" EMSCRIPTEN_KEEPALIVE char const * VV124F_Renderer_Name(void)
{
	return(Backend_Renderer_Name());
}


extern "C" EMSCRIPTEN_KEEPALIVE void VV124F_Finish(void)
{
	if (_Ready) {
		Backend_Shutdown();
		_Ready = false;
	}

	if (_Window != nullptr) {
		DestroyWindow(_Window);
		_Window = nullptr;
	}

	SDL_Quit();
}
