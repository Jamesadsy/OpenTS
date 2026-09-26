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


inline bool Cursor_Overlay_Should_Draw(bool cursor_visible, bool pointer_drawn)
{
	return(cursor_visible && pointer_drawn);
}


struct CursorContentSelection
{
	int Width = 0;
	int Height = 0;
	int Scale = 0;
	uint64_t Hash = 0;

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


struct CursorPresentationSnapshot
{
	int SemanticMouseType = 0;
	void const * Shape = nullptr;
	int Frame = 0;
	int NativeHotX = 0;
	int NativeHotY = 0;
	int DisplayScale = 0;
	int ImageWidth = 0;
	int ImageHeight = 0;
	uint64_t ContentHash = 0;
	uint64_t ContentGeneration = 0;
	int DrawableAnchorX = 0;
	int DrawableAnchorY = 0;
	int DestinationX = 0;
	int DestinationY = 0;

	bool operator==(CursorPresentationSnapshot const &) const = default;
};


inline CursorPresentationSnapshot Make_Cursor_Presentation_Snapshot(int semantic_mouse_type,
	void const * shape, int frame, int native_hot_x, int native_hot_y, int display_scale,
	int image_width, int image_height, uint64_t content_hash, uint64_t content_generation,
	int drawable_anchor_x, int drawable_anchor_y)
{
	CursorPresentationSnapshot snapshot;
	snapshot.SemanticMouseType = semantic_mouse_type;
	snapshot.Shape = shape;
	snapshot.Frame = frame;
	snapshot.NativeHotX = native_hot_x;
	snapshot.NativeHotY = native_hot_y;
	snapshot.DisplayScale = display_scale;
	snapshot.ImageWidth = image_width;
	snapshot.ImageHeight = image_height;
	snapshot.ContentHash = content_hash;
	snapshot.ContentGeneration = content_generation;
	snapshot.DrawableAnchorX = drawable_anchor_x;
	snapshot.DrawableAnchorY = drawable_anchor_y;
	snapshot.DestinationX = drawable_anchor_x - native_hot_x * display_scale;
	snapshot.DestinationY = drawable_anchor_y - native_hot_y * display_scale;
	return(snapshot);
}


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
