/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <cstdint>


struct CursorContentSelection
{
	void const * Shape = nullptr;
	int Frame = 0;
	int HotX = 0;
	int HotY = 0;
	int Scale = 0;

	bool operator==(CursorContentSelection const &) const = default;
};


class CursorContentGeneration
{
	public:
		bool Select(CursorContentSelection const & selection, bool rebuilt = false)
		{
			if (_HasSelection && _Selection == selection && !rebuilt) {
				return(false);
			}

			_Selection = selection;
			_HasSelection = true;
			++_Generation;
			if (_Generation == 0) {
				_Generation = 1;
			}
			return(true);
		}

		uint64_t Current(void) const
		{
			return(_Generation);
		}

		bool Needs_Present(void) const
		{
			return(_Generation != _PresentedGeneration);
		}

		bool Acknowledge(uint64_t generation)
		{
			if (_Generation == 0 || generation != _Generation) {
				return(false);
			}

			_PresentedGeneration = generation;
			return(true);
		}

	private:
		CursorContentSelection _Selection;
		uint64_t _Generation = 0;
		uint64_t _PresentedGeneration = 0;
		bool _HasSelection = false;
};


struct CursorTextureUploadState
{
	int Width = 0;
	int Height = 0;
	uint64_t Generation = 0;
	bool HasTexture = false;
};


inline bool Cursor_Texture_Needs_Upload(CursorTextureUploadState const & state,
	int width, int height, uint64_t generation)
{
	return(!state.HasTexture || state.Width != width || state.Height != height
		|| state.Generation != generation);
}
