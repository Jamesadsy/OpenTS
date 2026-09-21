/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "wincursor.h"

#include "_convert.h"
#include "_xmouse.h"
#include "convert.h"
#include "globals.h"
#include "goptions.h"
#include "shapeset.h"
#include "vidscale.h"
#include "video.h"
#include "win.h"
#include "winstub.h"
#include "xmouse.h"

#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>


struct CursorImage
{
	int Width = 0;
	int Height = 0;
	int HotX = 0;
	int HotY = 0;
	std::vector<unsigned char> Pixels;
};


struct CursorCacheEntry
{
	ShapeSet const * Shape;
	int Frame;
	int HotX;
	int HotY;
	HCURSOR Cursor;
	CursorImage Image;
};

// One cursor per shape frame the game actually asks for. MOUSE.SHP holds a few hundred
// of them, so the cache is emptied rather than grown when it fills.
static CursorCacheEntry _CursorCache[384];
static int _CursorCacheCount = 0;
static int _CacheScale = 0;

static ShapeSet const * _CurrentShape = NULL;
static int _CurrentFrame = 0;
static int _CurrentHotX = 0;
static int _CurrentHotY = 0;
static HCURSOR _CurrentCursor = NULL;
static CursorImage const * _CurrentImage = NULL;
static bool _CursorVisible = true;
static bool _OverlayDirty = true;
static int _PresentedX = 0;
static int _PresentedY = 0;
static bool _PresentedPositionValid = false;


/// <summary>
/// Works out how much larger than its shape the cursor should be drawn.
/// </summary>
/// <returns>int; A whole multiple between one and eight.</returns>
static int Cursor_Scale(void)
{
	if (Options.CursorScale < 0) {
		return(1);
	}

	if (Options.CursorScale > 0) {
		return(Options.CursorScale > 8 ? 8 : Options.CursorScale);
	}

	VideoScaleInfo const & scale = Video_Get_Scale_Info();
	float smaller = scale.ScaleX < scale.ScaleY ? scale.ScaleX : scale.ScaleY;

	int result = (int)(smaller + 0.5f);
	if (result < 1) result = 1;
	if (result > 8) result = 8;
	return(result);
}


/// <summary>
/// Draws one shape frame into a Windows cursor.
/// The canvas covers the shape's whole frame rather than the trimmed part that holds
/// pixels, so the hotspot, which is measured from the frame's corner, still lands in the
/// right place. Palette entry zero is the transparent one.
/// </summary>
/// <returns>The cursor, or NULL if it could not be built.</returns>
static bool Build_Cursor_Image(ShapeSet const * shape, int frame, int hotx, int hoty, int scale, CursorImage & image)
{
	image = CursorImage();

	if (shape == NULL || MouseDrawer == NULL) {
		return(false);
	}

	Rect rect = shape->Get_Rect(frame);
	unsigned char const * data = (unsigned char const *)shape->Get_Data(frame);

	if (!rect.Is_Valid() || data == NULL) {
		return(false);
	}

	int width = shape->Get_Width() * scale;
	int height = shape->Get_Height() * scale;

	if (width <= 0 || height <= 0) {
		return(false);
	}

	image.Width = width;
	image.Height = height;
	image.HotX = hotx * scale;
	image.HotY = hoty * scale;
	image.Pixels.assign((std::size_t)width * height * 4, 0);

	unsigned short const * table = (unsigned short const *)MouseDrawer->Get_Translate_Table();
	bool const compressed = shape->Is_RLE_Compressed(frame);
	unsigned char const * line = data;

	for (int y = 0; y < rect.Height; y++) {
		unsigned char const * source = compressed ? line + sizeof(unsigned short) : data + y * rect.Width;
		int x = 0;

		while (x < rect.Width) {
			unsigned char index = *source++;

			if (index == 0) {
				x += compressed ? *source++ : 1;
				continue;
			}

			unsigned short pixel = table[index];
			unsigned char red = (unsigned char)(((pixel >> 11) & 0x1F) << 3);
			unsigned char green = (unsigned char)(((pixel >> 5) & 0x3F) << 2);
			unsigned char blue = (unsigned char)((pixel & 0x1F) << 3);

			for (int suby = 0; suby < scale; suby++) {
				unsigned char * row = image.Pixels.data()
					+ ((std::size_t)((rect.Y + y) * scale + suby) * width + (std::size_t)(rect.X + x) * scale) * 4;
				for (int subx = 0; subx < scale; subx++) {
					row[subx * 4 + 0] = red;
					row[subx * 4 + 1] = green;
					row[subx * 4 + 2] = blue;
					row[subx * 4 + 3] = 255;
				}
			}

			x++;
		}

		if (compressed) {
			line += *(unsigned short const *)line;
		}
	}

	return(true);
}


static HCURSOR Build_Cursor(CursorImage const & image)
{
	if (image.Pixels.empty() || image.Width <= 0 || image.Height <= 0) {
		return(NULL);
	}

	int width = image.Width;
	int height = image.Height;

	BITMAPINFO info;
	memset(&info, '\0', sizeof(info));
	info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	info.bmiHeader.biWidth = width;
	info.bmiHeader.biHeight = -height;
	info.bmiHeader.biPlanes = 1;
	info.bmiHeader.biBitCount = 32;
	info.bmiHeader.biCompression = BI_RGB;

	void * bits = NULL;
	HBITMAP color = CreateDIBSection(NULL, &info, DIB_RGB_COLORS, &bits, NULL, 0);

	if (color == NULL) {
		return(NULL);
	}

	memset(bits, '\0', width * height * 4);
	for (int y = 0; y < height; y++) {
		unsigned char const * source = image.Pixels.data() + (std::size_t)y * width * 4;
		unsigned char * target = (unsigned char *)bits + (std::size_t)y * width * 4;
		for (int x = 0; x < width; x++) {
			target[x * 4 + 0] = source[x * 4 + 2];
			target[x * 4 + 1] = source[x * 4 + 1];
			target[x * 4 + 2] = source[x * 4 + 0];
			target[x * 4 + 3] = source[x * 4 + 3];
		}
	}

	// A color cursor carries its transparency in the alpha channel, but Windows still
	// wants a mask bitmap alongside it.
	int mask_pitch = ((width + 15) / 16) * 2;
	char * mask_bits = new char[mask_pitch * height];
	memset(mask_bits, '\0', mask_pitch * height);
	HBITMAP mask = CreateBitmap(width, height, 1, 1, mask_bits);
	delete [] mask_bits;

	int cursor_hotx = image.HotX;
	int cursor_hoty = image.HotY;
	if (cursor_hotx < 0) cursor_hotx = 0;
	if (cursor_hoty < 0) cursor_hoty = 0;
	if (cursor_hotx >= width) cursor_hotx = width - 1;
	if (cursor_hoty >= height) cursor_hoty = height - 1;

	ICONINFO icon;
	icon.fIcon = FALSE;
	icon.xHotspot = cursor_hotx;
	icon.yHotspot = cursor_hoty;
	icon.hbmMask = mask;
	icon.hbmColor = color;
	HCURSOR cursor = (HCURSOR)CreateIconIndirect(&icon);

	DeleteObject(mask);
	DeleteObject(color);
	return(cursor);
}


static void Flush_Cursor_Cache(void)
{
	for (int index = 0; index < _CursorCacheCount; index++) {
		if (_CursorCache[index].Cursor != NULL) {
			DestroyCursor(_CursorCache[index].Cursor);
		}
	}

	_CursorCacheCount = 0;
	_CurrentCursor = NULL;
	_CurrentImage = NULL;
	_OverlayDirty = true;
	_PresentedPositionValid = false;
}


/// <summary>
/// Selects the cursor for a shape frame, building it if it has not been seen before.
/// </summary>
/// <param name="shape">The shape set the frame belongs to.</param>
/// <param name="frame">Which frame of it to show.</param>
/// <param name="hotx">The point within the frame that does the pointing.</param>
/// <param name="hoty">The same, vertically.</param>
/// <param name="apply">Should the cursor be shown straight away?</param>
void Win_Cursor_Set(ShapeSet const * shape, int frame, int hotx, int hoty, bool apply)
{
	int scale = Cursor_Scale();

	if (scale != _CacheScale) {
		Flush_Cursor_Cache();
		_CacheScale = scale;
	}

	_CurrentShape = shape;
	_CurrentFrame = frame;
	_CurrentHotX = hotx;
	_CurrentHotY = hoty;

	HCURSOR cursor = NULL;
	bool selected = false;

	for (int index = 0; index < _CursorCacheCount; index++) {
		CursorCacheEntry & entry = _CursorCache[index];
		if (entry.Shape == shape && entry.Frame == frame) {
			if (entry.HotX != hotx || entry.HotY != hoty || entry.Image.Pixels.empty()) {
				if (entry.Cursor != NULL) {
					DestroyCursor(entry.Cursor);
				}
				CursorImage image;
				Build_Cursor_Image(shape, frame, hotx, hoty, scale, image);
				entry.Cursor = Build_Cursor(image);
				entry.Image = std::move(image);
				entry.HotX = hotx;
				entry.HotY = hoty;
			}
			cursor = entry.Cursor;
			_CurrentImage = entry.Image.Pixels.empty() ? NULL : &entry.Image;
			selected = true;
			break;
		}
	}

	if (!selected) {
		if (_CursorCacheCount >= (int)(sizeof(_CursorCache) / sizeof(_CursorCache[0]))) {
			Flush_Cursor_Cache();
		}

		CursorImage image;
		Build_Cursor_Image(shape, frame, hotx, hoty, scale, image);
		cursor = Build_Cursor(image);

		if (!image.Pixels.empty()) {
			CursorCacheEntry & entry = _CursorCache[_CursorCacheCount++];
			entry.Shape = shape;
			entry.Frame = frame;
			entry.HotX = hotx;
			entry.HotY = hoty;
			entry.Cursor = cursor;
			entry.Image = std::move(image);
			_CurrentImage = &entry.Image;
		} else {
			_CurrentImage = NULL;
		}
	}

	_CurrentCursor = cursor;
	_OverlayDirty = true;

	if (apply) {
		SetCursor(_CursorVisible ? _CurrentCursor : NULL);
	}
}


/// <summary>
/// Shows or hides the pointer.
/// </summary>
void Win_Cursor_Set_Visible(bool visible)
{
	if (_CursorVisible != visible) {
		_OverlayDirty = true;
	}
	_CursorVisible = visible;

	if (MouseCursor != NULL && MouseCursor->Is_Captured()) {
		SetCursor(visible ? _CurrentCursor : NULL);
	}
}


/// <summary>
/// Puts the game's pointer back after Windows has asked what the cursor should be.
/// </summary>
/// <returns>bool; Was the cursor the game's to choose? While a dialog has the mouse it
/// is not, and Windows keeps its own arrow.</returns>
bool Win_Cursor_Handle_Set_Cursor(void)
{
	if (MouseCursor == NULL || !MouseCursor->Is_Captured()) {
		return(false);
	}

	SetCursor(_CursorVisible ? _CurrentCursor : NULL);
	return(true);
}


/// <summary>
/// Rebuilds the pointer if the window has been resized enough to want a different size.
/// </summary>
void Win_Cursor_Refresh(void)
{
	if (_CurrentShape != NULL && Cursor_Scale() != _CacheScale) {
		Win_Cursor_Set(_CurrentShape, _CurrentFrame, _CurrentHotX, _CurrentHotY,
			MouseCursor != NULL && MouseCursor->Is_Captured());
	}
}


static bool Current_Overlay_Position(int * x, int * y)
{
#ifdef OPENTS_IOS
	if (x == NULL || y == NULL || MouseCursor == NULL || !MouseCursor->Is_Captured()) {
		return(false);
	}

	Point2D const game_point = MouseCursor->Get_Mouse_Point();
	POINT window_point;
	window_point.x = game_point.X;
	window_point.y = game_point.Y;
	Game_Point_To_Window(window_point);
	*x = window_point.x;
	*y = window_point.y;
	return(true);
#else
	(void)x;
	(void)y;
	return(false);
#endif
}


bool Win_Cursor_Get_Overlay(WinCursorOverlay * overlay)
{
#ifdef OPENTS_IOS
	if (overlay == NULL || !_CursorVisible || Win_Pointer_Is_Direct_Touch()
		|| _CurrentImage == NULL
		|| _CurrentImage->Pixels.empty()) {
		return(false);
	}

	int x = 0;
	int y = 0;
	if (!Current_Overlay_Position(&x, &y)) {
		return(false);
	}

	overlay->Pixels = _CurrentImage->Pixels.data();
	overlay->Width = _CurrentImage->Width;
	overlay->Height = _CurrentImage->Height;
	overlay->HotX = _CurrentImage->HotX;
	overlay->HotY = _CurrentImage->HotY;
	overlay->X = x - overlay->HotX;
	overlay->Y = y - overlay->HotY;
	return(true);
#else
	(void)overlay;
	return(false);
#endif
}


bool Win_Cursor_Is_Dirty(void)
{
#ifdef OPENTS_IOS
	if (_OverlayDirty) {
		return(true);
	}

	WinCursorOverlay overlay;
	if (!Win_Cursor_Get_Overlay(&overlay)) {
		return(_PresentedPositionValid);
	}

	return(!_PresentedPositionValid
		|| overlay.X + overlay.HotX != _PresentedX
		|| overlay.Y + overlay.HotY != _PresentedY);
#else
	return(false);
#endif
}


void Win_Cursor_Acknowledge_Present(void)
{
#ifdef OPENTS_IOS
	WinCursorOverlay overlay;
	if (Win_Cursor_Get_Overlay(&overlay)) {
		_PresentedX = overlay.X + overlay.HotX;
		_PresentedY = overlay.Y + overlay.HotY;
		_PresentedPositionValid = true;
	} else {
		_PresentedPositionValid = false;
	}
	_OverlayDirty = false;
#endif
}


/// <summary>
/// Releases every cursor the game built.
/// </summary>
void Win_Cursor_Shutdown(void)
{
	SetCursor(NULL);
	Flush_Cursor_Cache();
	_CurrentShape = NULL;
	_CurrentImage = NULL;
}
