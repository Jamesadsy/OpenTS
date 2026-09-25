/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

enum class ControllerMenuMode
{
	POINTER,
	FOCUS
};

enum class ControllerMenuRoute
{
	POINTER,
	FOCUS,
	MOVIE
};


class ControllerMenuOwnershipModel
{
	public:
		void Set_Menu_Surface(bool active)
		{
			_MenuSurfaceActive = active;
			_Mode = ControllerMenuMode::POINTER;
		}

		void Reset(void)
		{
			_Mode = ControllerMenuMode::POINTER;
		}

		void Reset_Mode(void)
		{
			_Mode = ControllerMenuMode::POINTER;
		}

		void Dpad_Navigated(void)
		{
			if (_MenuSurfaceActive) {
				_Mode = ControllerMenuMode::FOCUS;
			}
		}

		void Pointer_Moved(void)
		{
			if (_MenuSurfaceActive) {
				_Mode = ControllerMenuMode::POINTER;
			}
		}

		bool Menu_Surface_Active(void) const
		{
			return(_MenuSurfaceActive);
		}

		bool Focus_Owns_Menu(void) const
		{
			return(_MenuSurfaceActive && _Mode == ControllerMenuMode::FOCUS);
		}

		ControllerMenuMode Mode(void) const
		{
			return(_Mode);
		}

		ControllerMenuRoute Cross_Route(void) const
		{
			return(Focus_Owns_Menu() ? ControllerMenuRoute::FOCUS : ControllerMenuRoute::POINTER);
		}

		ControllerMenuRoute Circle_Route(bool movie) const
		{
			if (movie) {
				return(ControllerMenuRoute::MOVIE);
			}
			return(Focus_Owns_Menu() ? ControllerMenuRoute::FOCUS : ControllerMenuRoute::POINTER);
		}

	private:
		bool _MenuSurfaceActive = false;
		ControllerMenuMode _Mode = ControllerMenuMode::POINTER;
};
