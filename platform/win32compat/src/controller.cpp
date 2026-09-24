/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "win32compat.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <deque>

namespace
{

constexpr Sint16 GAMEPAD_AXIS_MAX = 32767;
constexpr Sint16 GAMEPAD_DEAD_ZONE = 4000;
constexpr double GAMEPAD_AXIS_EXPONENT = 1.03;
constexpr double GAMEPAD_CURSOR_SPEED = 240.0;
constexpr double GAMEPAD_CAMERA_SPEED = 480.0;
constexpr double GAMEPAD_TRIGGER_MAX = 32767.0;
constexpr double GAMEPAD_STALL_RESET_SECONDS = 0.1;

SDL_Gamepad * _Gamepad;
SDL_JoystickID _GamepadId;
Sint16 _Axes[SDL_GAMEPAD_AXIS_COUNT];
bool _Buttons[SDL_GAMEPAD_BUTTON_COUNT];
bool _ControllerControlHeld;
bool _ControllerAltHeld;
bool _MovieCircleOwned;
bool _Focused = true;
bool _Initialized;
bool _HaveServiceTime;
Uint64 _LastService;
double _CameraPanX;
double _CameraPanY;

#ifdef OPENTS_GAMEPAD_TEST
struct TestKeyEvent
{
	int VirtualKey;
	bool Down;
};

bool _TestMode;
bool _TestConnected;
bool _TestWindowSizeEnabled;
float _TestWindowWidth;
float _TestWindowHeight;
Uint64 _TestNow;
std::deque<TestKeyEvent> _TestKeyEvents;
#endif

std::deque<int> _Actions;


double Axis_Value(Sint16 value)
{
	double const magnitude = std::min(1.0, std::abs((double)value) / (double)GAMEPAD_AXIS_MAX);
	double const dead = (double)GAMEPAD_DEAD_ZONE / (double)GAMEPAD_AXIS_MAX;

	if (magnitude <= dead) {
		return(0.0);
	}

	double const normalized = (magnitude - dead) / (1.0 - dead);
	double const shaped = std::pow(normalized, GAMEPAD_AXIS_EXPONENT);
	return(value < 0 ? -shaped : shaped);
}


double Trigger_Value(Sint16 value)
{
	return(std::clamp((double)value, 0.0, GAMEPAD_TRIGGER_MAX) / GAMEPAD_TRIGGER_MAX);
}


Uint64 Controller_Now(void)
{
#ifdef OPENTS_GAMEPAD_TEST
	if (_TestMode) {
		return(_TestNow);
	}
#endif
	return(SDL_GetTicksNS());
}


bool Window_Size(float & width, float & height)
{
#ifdef OPENTS_GAMEPAD_TEST
	if (_TestMode && _TestWindowSizeEnabled) {
		width = _TestWindowWidth;
		height = _TestWindowHeight;
		return(width > 0.0f && height > 0.0f);
	}
#endif

	Win32Window * main = Win32_Lookup(Win32_Main_Window());

	if (main == NULL || main->Handle == NULL) {
		return(false);
	}

	int w = 0;
	int h = 0;

	if (!SDL_GetWindowSize(main->Handle, &w, &h) || w <= 0 || h <= 0) {
		return(false);
	}

	width = (float)w;
	height = (float)h;
	return(true);
}


bool Controller_Active(void)
{
#ifdef OPENTS_GAMEPAD_TEST
	if (_TestMode) {
		return(_TestConnected);
	}
#endif
	return(_Gamepad != NULL);
}


void Post_Controller_Key(int virtualkey, bool down)
{
#ifdef OPENTS_GAMEPAD_TEST
	if (_TestMode) {
		_TestKeyEvents.push_back(TestKeyEvent{ virtualkey, down });
	}
#endif
	Win32_Post_Key_Message(virtualkey, down);
}


void Post_Controller_Mouse(Uint8 button, bool down, UINT message)
{
	if (Win32_Pointer_Controller_Button(button, down)) {
		Win32_Post_Pointer_Message(message);
	}
}


int Dpad_Key(SDL_GamepadButton button)
{
	switch (button) {
		case SDL_GAMEPAD_BUTTON_DPAD_UP: return(WIN32_VK_UP);
		case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return(WIN32_VK_DOWN);
		case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return(WIN32_VK_LEFT);
		case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return(WIN32_VK_RIGHT);
		default: return(0);
	}
}


void Apply_Button(SDL_GamepadButton button, bool down)
{
	int const index = (int)button;
	if (index < 0 || index >= SDL_GAMEPAD_BUTTON_COUNT || _Buttons[index] == down) {
		return;
	}

	_Buttons[index] = down;

	if (down) {
		switch (button) {
			case SDL_GAMEPAD_BUTTON_WEST:
				_Actions.push_back(WIN32_GAMEPAD_ACTION_SQUARE);
				break;

			case SDL_GAMEPAD_BUTTON_NORTH:
				_Actions.push_back(WIN32_GAMEPAD_ACTION_TRIANGLE);
				break;

			default:
				break;
		}
	}

	if (down) {
		Win32_Pointer_Set_Direct_Touch(false);
	}

	switch (button) {
		case SDL_GAMEPAD_BUTTON_SOUTH:
			Post_Controller_Mouse(SDL_BUTTON_LEFT, down,
				down ? WM_LBUTTONDOWN : WM_LBUTTONUP);
			break;

		case SDL_GAMEPAD_BUTTON_EAST:
			if (down && Win32_Touch_Movie_Mode()) {
				_MovieCircleOwned = true;
				Win32_Touch_Movie_Circle(true);
			} else if (!down && _MovieCircleOwned) {
				Win32_Touch_Movie_Circle(false);
				_MovieCircleOwned = false;
			} else {
				Post_Controller_Mouse(SDL_BUTTON_RIGHT, down,
					down ? WM_RBUTTONDOWN : WM_RBUTTONUP);
			}
			break;

		case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
			_ControllerControlHeld = down;
			Win32_Controller_Key(WIN32_VK_CONTROL, down);
			Post_Controller_Key(WIN32_VK_CONTROL, down);
			break;

		case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
			_ControllerAltHeld = down;
			Win32_Controller_Key(WIN32_VK_MENU, down);
			Post_Controller_Key(WIN32_VK_MENU, down);
			break;

		case SDL_GAMEPAD_BUTTON_DPAD_UP:
		case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
		case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
		case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: {
			int const key = Dpad_Key(button);
			if (key != 0) {
				Post_Controller_Key(key, down);
			}
			break;
		}

		case SDL_GAMEPAD_BUTTON_START:
			if (down) {
				Post_Controller_Key(WIN32_VK_ESCAPE, true);
				Post_Controller_Key(WIN32_VK_ESCAPE, false);
			}
			break;

		default:
			// Back, Guide and the other symbolic buttons remain inert until the
			// engine has a source-backed action for them.
			break;
	}
}


void Reset_Device_State(void)
{
	if (_MovieCircleOwned) {
		Win32_Touch_Movie_Circle(false);
	}
	Win32_Pointer_Set_Controller_Owner(false);
	std::memset(_Axes, 0, sizeof(_Axes));
	std::memset(_Buttons, 0, sizeof(_Buttons));
	_ControllerControlHeld = false;
	_ControllerAltHeld = false;
	_MovieCircleOwned = false;
	_CameraPanX = 0.0;
	_CameraPanY = 0.0;
	_HaveServiceTime = false;
	_LastService = 0;
	_Actions.clear();
}


void Read_Gamepad_State(void)
{
	if (_Gamepad == NULL) {
		return;
	}

	for (int axis = 0; axis < SDL_GAMEPAD_AXIS_COUNT; axis++) {
		_Axes[axis] = SDL_GetGamepadAxis(_Gamepad, (SDL_GamepadAxis)axis);
	}

	for (int button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; button++) {
		Apply_Button((SDL_GamepadButton)button,
			SDL_GetGamepadButton(_Gamepad, (SDL_GamepadButton)button));
	}
}


void Close_Gamepad(void)
{
	if (_Gamepad != NULL) {
		SDL_CloseGamepad(_Gamepad);
		_Gamepad = NULL;
	}
	_GamepadId = 0;
}


bool Open_Gamepad(SDL_JoystickID id)
{
	if (!SDL_IsGamepad(id)) {
		return(false);
	}

	SDL_Gamepad * gamepad = SDL_OpenGamepad(id);
	if (gamepad == NULL) {
		return(false);
	}

	_Gamepad = gamepad;
	_GamepadId = id;
	Reset_Device_State();
	Read_Gamepad_State();
	return(true);
}


void Open_First_Gamepad(void)
{
	int count = 0;
	SDL_JoystickID * ids = SDL_GetGamepads(&count);

	if (ids == NULL) {
		return;
	}

	for (int index = 0; index < count; index++) {
		if (Open_Gamepad(ids[index])) {
			break;
		}
	}

	SDL_free(ids);
}

} // namespace


void Win32_Gamepad_Initialize(void)
{
	if (_Initialized) {
		return;
	}

	_Initialized = true;
	_Focused = true;
	Reset_Device_State();

#ifdef OPENTS_GAMEPAD_TEST
	if (_TestMode) {
		return;
	}
#endif

	if (SDL_WasInit(SDL_INIT_GAMEPAD) != 0) {
		Open_First_Gamepad();
	}
}


void Win32_Gamepad_Handle_Event(SDL_Event const & event)
{
	if (!_Initialized) {
		return;
	}

	switch (event.type) {
		case SDL_EVENT_GAMEPAD_ADDED:
			if (_Gamepad == NULL) {
				Open_Gamepad(event.gdevice.which);
			}
			break;

		case SDL_EVENT_GAMEPAD_REMOVED:
			if (_Gamepad != NULL && event.gdevice.which == _GamepadId) {
				Win32_Gamepad_Release_All();
				Close_Gamepad();
				Open_First_Gamepad();
			}
			break;

		case SDL_EVENT_GAMEPAD_REMAPPED:
			if (_Gamepad != NULL && event.gdevice.which == _GamepadId) {
				Read_Gamepad_State();
			}
			break;

		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			if (_Gamepad != NULL && event.gaxis.which == _GamepadId
				&& event.gaxis.axis < SDL_GAMEPAD_AXIS_COUNT) {
				_Axes[event.gaxis.axis] = event.gaxis.value;
			}
			break;

		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
			if (_Gamepad != NULL && event.gbutton.which == _GamepadId
				&& event.gbutton.button < SDL_GAMEPAD_BUTTON_COUNT) {
				Apply_Button((SDL_GamepadButton)event.gbutton.button,
					event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
			}
			break;

		default:
			break;
	}
}


void Win32_Gamepad_Service(void)
{
	if (!_Initialized || !_Focused || !Controller_Active()) {
		return;
	}

	Uint64 const now = Controller_Now();
	if (!_HaveServiceTime) {
		_LastService = now;
		_HaveServiceTime = true;
		return;
	}

	double elapsed = (double)(now - _LastService) / 1000000000.0;
	_LastService = now;
	if (elapsed <= 0.0) {
		return;
	}
	if (elapsed > GAMEPAD_STALL_RESET_SECONDS) {
		// A service gap means the host was not presenting. Do not turn the whole gap into
		// one visible teleport when the next outer frame arrives.
		elapsed = 0.0;
	}

	if (_Gamepad != NULL) {
		for (int axis = 0; axis < SDL_GAMEPAD_AXIS_COUNT; axis++) {
			_Axes[axis] = SDL_GetGamepadAxis(_Gamepad, (SDL_GamepadAxis)axis);
		}
		for (int button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; button++) {
			Apply_Button((SDL_GamepadButton)button,
				SDL_GetGamepadButton(_Gamepad, (SDL_GamepadButton)button));
		}
	}

	double const pointer_boost = 1.0 + 2.0 * Trigger_Value(_Axes[SDL_GAMEPAD_AXIS_RIGHT_TRIGGER]);
	double const leftx = Axis_Value(_Axes[SDL_GAMEPAD_AXIS_LEFTX]);
	double const lefty = Axis_Value(_Axes[SDL_GAMEPAD_AXIS_LEFTY]);
	double const rightx = Axis_Value(_Axes[SDL_GAMEPAD_AXIS_RIGHTX]);
	double const righty = Axis_Value(_Axes[SDL_GAMEPAD_AXIS_RIGHTY]);

	float width = 0.0f;
	float height = 0.0f;
	if ((leftx != 0.0 || lefty != 0.0) && Window_Size(width, height)) {
		float x = 0.0f;
		float y = 0.0f;
		Win32_Pointer_Position(&x, &y);
		float const old_x = x;
		float const old_y = y;
		x = std::clamp((double)x + leftx * GAMEPAD_CURSOR_SPEED * elapsed * pointer_boost,
			0.0, (double)width - 1.0);
		y = std::clamp((double)y + lefty * GAMEPAD_CURSOR_SPEED * elapsed * pointer_boost,
			0.0, (double)height - 1.0);
		if (x != old_x || y != old_y) {
			Win32_Pointer_Set_Direct_Touch(false);
			Win32_Pointer_Move_Controller(x, y);
			Win32_Post_Pointer_Message(WM_MOUSEMOVE);
		}
	}

	if (rightx != 0.0 || righty != 0.0) {
		Win32_Pointer_Set_Direct_Touch(false);
		_CameraPanX += rightx * GAMEPAD_CAMERA_SPEED * elapsed;
		_CameraPanY += righty * GAMEPAD_CAMERA_SPEED * elapsed;
	}
}


void Win32_Gamepad_Set_Focus(bool focused)
{
	if (!focused) {
		Win32_Gamepad_Release_All();
		_Focused = false;
		return;
	}

	_Focused = true;
	_HaveServiceTime = false;
}


void Win32_Gamepad_Release_All(void)
{
	for (int button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; button++) {
		if (_Buttons[button]) {
			Apply_Button((SDL_GamepadButton)button, false);
		}
	}

	Reset_Device_State();
}


bool Win32_Gamepad_Take_Camera_Pan(int * x, int * y)
{
	int const dx = (int)_CameraPanX;
	int const dy = (int)_CameraPanY;
	_CameraPanX -= (double)dx;
	_CameraPanY -= (double)dy;

	if (x != NULL) *x = dx;
	if (y != NULL) *y = dy;
	return(dx != 0 || dy != 0);
}


bool Win32_Gamepad_Camera_Pan_Active(void)
{
	return(Controller_Active()
		&& (Axis_Value(_Axes[SDL_GAMEPAD_AXIS_RIGHTX]) != 0.0
			|| Axis_Value(_Axes[SDL_GAMEPAD_AXIS_RIGHTY]) != 0.0
			|| (int)_CameraPanX != 0 || (int)_CameraPanY != 0));
}


Uint64 Win32_Monotonic_Time_Ms(void)
{
	return(SDL_GetTicksNS() / 1000000ULL);
}


bool Win32_Gamepad_Take_Action(int * action)
{
	if (_Actions.empty()) {
		return(false);
	}

	int const next = _Actions.front();
	_Actions.pop_front();
	if (action != NULL) {
		*action = next;
	}
	return(true);
}


void Win32_Gamepad_Discard_Actions(void)
{
	_Actions.clear();
}


void Win32_Gamepad_Shutdown(void)
{
	Win32_Gamepad_Release_All();
	Close_Gamepad();
	_Initialized = false;
	_Focused = true;
}


#ifdef OPENTS_GAMEPAD_TEST
void Win32_Gamepad_Test_Reset(void)
{
	Win32_Gamepad_Release_All();
	if (_Gamepad != NULL) {
		Close_Gamepad();
	}

	_Initialized = true;
	_Focused = true;
	_TestMode = true;
	_TestConnected = false;
	_TestWindowSizeEnabled = false;
	_TestWindowWidth = 0.0f;
	_TestWindowHeight = 0.0f;
	_TestNow = 0;
	_TestKeyEvents.clear();
	Reset_Device_State();
	Win32_Pointer_Button(SDL_BUTTON_LEFT, false);
	Win32_Pointer_Button(SDL_BUTTON_RIGHT, false);
	Win32_Pointer_Set_Direct_Touch(false);
}


void Win32_Gamepad_Test_Set_Window_Size(float width, float height)
{
	_TestWindowSizeEnabled = true;
	_TestWindowWidth = width;
	_TestWindowHeight = height;
}


void Win32_Gamepad_Test_Set_Now(Uint64 now)
{
	_TestNow = now;
}


void Win32_Gamepad_Test_Set_Connected(bool connected)
{
	Win32_Gamepad_Release_All();
	_TestConnected = connected;
	Reset_Device_State();
}


void Win32_Gamepad_Test_Set_Axis(SDL_GamepadAxis axis, Sint16 value)
{
	int const index = (int)axis;
	if (_TestConnected && index >= 0 && index < SDL_GAMEPAD_AXIS_COUNT) {
		_Axes[index] = value;
	}
}


void Win32_Gamepad_Test_Set_Button(SDL_GamepadButton button, bool down)
{
	int const index = (int)button;
	if (_TestConnected && index >= 0 && index < SDL_GAMEPAD_BUTTON_COUNT) {
		Apply_Button(button, down);
	}
}


void Win32_Gamepad_Test_Service(void)
{
	Win32_Gamepad_Service();
}


bool Win32_Gamepad_Test_Take_Key_Event(int * virtualkey, bool * down)
{
	if (_TestKeyEvents.empty()) {
		return(false);
	}

	TestKeyEvent const event = _TestKeyEvents.front();
	_TestKeyEvents.pop_front();
	if (virtualkey != NULL) *virtualkey = event.VirtualKey;
	if (down != NULL) *down = event.Down;
	return(true);
}
#endif
