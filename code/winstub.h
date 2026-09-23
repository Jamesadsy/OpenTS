/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

#pragma once

#include "point.h"
#include "win.h"

class Surface;
class PaletteClass;
struct NativeWindow;

void Create_Main_Window ( HINSTANCE instance , int command_show , int width , int height);
NativeWindow Win_Native_Window(HWND window);
bool Win_Window_Drawable_Size(HWND window, int & width, int & height);
bool Win_Preferred_Frame_Size(int & width, int & height);
#ifdef _WIN32
// Windows keeps both kinds of data beside the executable.  The false return is
// therefore the production behavior as well as the lightweight-test behavior.
inline bool Win_Log_Directory(char * path, int size)
{
	(void)path;
	(void)size;
	return(false);
}


inline bool Win_Shipped_Data_Directory(char * path, int size)
{
	(void)path;
	(void)size;
	return(false);
}
#else
bool Win_Log_Directory(char * path, int size);
bool Win_Shipped_Data_Directory(char * path, int size);
#endif
int Win_Window_Refresh_Rate(HWND window);
bool Win_Set_Window_Fullscreen(HWND window, bool fullscreen);
bool Win_Pointer_Can_Warp(void);
bool Win_Pointer_Is_Direct_Touch(void);
void Win_Pointer_Set_Direct_Touch(bool direct);
bool Win_Pointer_Should_Suppress_Edge_Scroll(void);
void Win_Text_Input_Begin(void);
void Win_Text_Input_End(void);
bool Win_Pointer_Is_Drawn(void);
bool Win_Window_Safe_Area(HWND window, RECT & area);
bool Win_Pointer_Take_Scroll(int & touch_x, int & touch_y, int & controller_x, int & controller_y);

enum WinGamepadAction {
	WIN_GAMEPAD_ACTION_NONE = 0,
	WIN_GAMEPAD_ACTION_SQUARE,
	WIN_GAMEPAD_ACTION_TRIANGLE
};

// The host services the controller once per outer frame. Gameplay consumes the two
// action edges through native engine endpoints; frontend/modal surfaces discard them.
void Win_Gamepad_Service(void);
bool Win_Gamepad_Take_Action(WinGamepadAction & action);
void Win_Gamepad_Discard_Actions(void);

void Win_Set_Movie_Playing(bool playing);
void Set_Window_Fullscreen(bool fullscreen);

extern Point2D TitleScreenOffset;

void Load_Title_Screen(char const * name, Surface * surface, PaletteClass * palette);

unsigned int Build_Number(void);
