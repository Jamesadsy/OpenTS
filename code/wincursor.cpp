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
#include "cursorpresentationpolicy.hh"
#include "dbgprint.h"
#include "globals.h"
#include "goptions.h"
#include "mouse.h"
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
	uint64_t ContentHash = 0;
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

static constexpr int STATIC_POINTER_FRAME = 0;
static constexpr int STATIC_POINTER_HOT_X = 0;
static constexpr int STATIC_POINTER_HOT_Y = 0;

static ShapeSet const * _CurrentShape = NULL;
static int _CurrentFrame = 0;
static int _CurrentHotX = 0;
static int _CurrentHotY = 0;
static MouseType _SemanticMouseType = MOUSE_NORMAL;
static HCURSOR _CurrentCursor = NULL;
static CursorImage const * _CurrentImage = NULL;
static CursorImage _StaticPointerImage;
static ShapeSet const * _StaticPointerShape = NULL;
static int _StaticPointerScale = 0;
static bool _CursorVisible = true;
static bool _OverlayDirty = true;
static CursorPresentationSnapshot _PresentedSnapshot;
static bool _PresentedSnapshotValid = false;
static CursorPresentationSnapshot _LastDiagnosticSnapshot;
static bool _DiagnosticSnapshotValid = false;
static CursorContentGeneration _ContentGeneration;


/// <summary>
/// Retains the native MouseClass selection for diagnostic snapshots. Raster presentation
/// updates intentionally leave this semantic value unchanged.
/// </summary>
void Win_Cursor_Set_Semantic_Mouse_Type(MouseType semantic_mouse_type)
{
	assert((unsigned)semantic_mouse_type < MOUSE_COUNT);
	if (_SemanticMouseType != semantic_mouse_type) {
		_SemanticMouseType = semantic_mouse_type;
		_OverlayDirty = true;
	}
}


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
static bool Build_Cursor_Image(ShapeSet const * shape, int frame, int scale, CursorImage & image)
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

	uint64_t hash = 14695981039346656037ULL;
	for (unsigned char pixel : image.Pixels) {
		hash ^= pixel;
		hash *= 1099511628211ULL;
	}
	image.ContentHash = hash;

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
	_StaticPointerImage = CursorImage();
	_StaticPointerShape = NULL;
	_StaticPointerScale = 0;
	_OverlayDirty = true;
	_PresentedSnapshotValid = false;
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
	bool cache_invalidated = false;

	if (scale != _CacheScale) {
		Flush_Cursor_Cache();
		_CacheScale = scale;
		cache_invalidated = true;
	}

	_CurrentShape = shape;
	_CurrentFrame = frame;
	_CurrentHotX = hotx;
	_CurrentHotY = hoty;
	bool const static_pointer = MouseCursor != NULL && MouseCursor->Is_Captured();
	int const display_frame = static_pointer ? STATIC_POINTER_FRAME : frame;
	int const display_hot_x = static_pointer ? STATIC_POINTER_HOT_X : hotx;
	int const display_hot_y = static_pointer ? STATIC_POINTER_HOT_Y : hoty;

	HCURSOR cursor = NULL;
	bool selected = false;

	for (int index = 0; index < _CursorCacheCount; index++) {
		CursorCacheEntry & entry = _CursorCache[index];
		if (entry.Shape == shape && entry.Frame == display_frame) {
			if (entry.HotX != display_hot_x || entry.HotY != display_hot_y || entry.Image.Pixels.empty()) {
				if (entry.Cursor != NULL) {
					DestroyCursor(entry.Cursor);
				}
				if (entry.Image.Pixels.empty()) {
					Build_Cursor_Image(shape, display_frame, scale, entry.Image);
				}
				entry.Image.HotX = display_hot_x * scale;
				entry.Image.HotY = display_hot_y * scale;
				entry.Cursor = Build_Cursor(entry.Image);
				entry.HotX = display_hot_x;
				entry.HotY = display_hot_y;
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
			cache_invalidated = true;
		}

		CursorImage image;
		Build_Cursor_Image(shape, display_frame, scale, image);
		image.HotX = display_hot_x * scale;
		image.HotY = display_hot_y * scale;
		cursor = Build_Cursor(image);

		if (!image.Pixels.empty()) {
			CursorCacheEntry & entry = _CursorCache[_CursorCacheCount++];
			entry.Shape = shape;
			entry.Frame = display_frame;
			entry.HotX = display_hot_x;
			entry.HotY = display_hot_y;
			entry.Cursor = cursor;
			entry.Image = std::move(image);
			_CurrentImage = &entry.Image;
		} else {
			_CurrentImage = NULL;
		}
	}

	if (_CurrentImage != NULL) {
		_ContentGeneration.Select(CursorContentSelection{
			_CurrentImage->Width, _CurrentImage->Height, scale, _CurrentImage->ContentHash
		}, cache_invalidated);
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
	if (x == NULL || y == NULL || MouseCursor == NULL) {
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


static void Trace_Cursor_Presentation(CursorPresentationSnapshot const & snapshot)
{
	bool const transition = !_DiagnosticSnapshotValid
		|| snapshot.SemanticMouseType != _LastDiagnosticSnapshot.SemanticMouseType
		|| snapshot.Shape != _LastDiagnosticSnapshot.Shape
		|| snapshot.Frame != _LastDiagnosticSnapshot.Frame
		|| snapshot.NativeHotX != _LastDiagnosticSnapshot.NativeHotX
		|| snapshot.NativeHotY != _LastDiagnosticSnapshot.NativeHotY
		|| snapshot.DisplayScale != _LastDiagnosticSnapshot.DisplayScale
		|| snapshot.ImageWidth != _LastDiagnosticSnapshot.ImageWidth
		|| snapshot.ImageHeight != _LastDiagnosticSnapshot.ImageHeight
		|| snapshot.ContentHash != _LastDiagnosticSnapshot.ContentHash
		|| snapshot.ContentGeneration != _LastDiagnosticSnapshot.ContentGeneration;

	if (transition) {
		DebugString("CursorSnapshot type=%d shape=%p frame=%d native_hot=%d,%d scale=%d canvas=%dx%d hash=%016llx generation=%llu anchor=%d,%d top_left=%d,%d\n",
			snapshot.SemanticMouseType, const_cast<void *>(snapshot.Shape), snapshot.Frame,
			snapshot.NativeHotX, snapshot.NativeHotY, snapshot.DisplayScale,
			snapshot.ImageWidth, snapshot.ImageHeight,
			(unsigned long long)snapshot.ContentHash,
			(unsigned long long)snapshot.ContentGeneration,
			snapshot.DrawableAnchorX, snapshot.DrawableAnchorY,
			snapshot.DestinationX, snapshot.DestinationY);
	}

	_LastDiagnosticSnapshot = snapshot;
	_DiagnosticSnapshotValid = true;
}


bool Win_Cursor_Get_Overlay(WinCursorOverlay * overlay)
{
#ifdef OPENTS_IOS
	if (overlay == NULL || !Cursor_Overlay_Should_Draw(_CursorVisible, Win_Pointer_Is_Drawn())
		|| MouseCursor == NULL) {
		return(false);
	}
	bool const front_end = !MouseCursor->Is_Captured();
	if (_CurrentShape == NULL) {
		return(false);
	}
	if (_StaticPointerShape != _CurrentShape || _StaticPointerScale != _CacheScale
		|| _StaticPointerImage.Pixels.empty()) {
		Build_Cursor_Image(_CurrentShape, STATIC_POINTER_FRAME, _CacheScale, _StaticPointerImage);
		_StaticPointerShape = _CurrentShape;
		_StaticPointerScale = _CacheScale;
	}
	CursorImage const * image = &_StaticPointerImage;
	if (image->Pixels.empty()) {
		return(false);
	}

	int x = 0;
	int y = 0;
	if (!Current_Overlay_Position(&x, &y)) {
		return(false);
	}

	_ContentGeneration.Select(CursorContentSelection{
		image->Width, image->Height, _CacheScale, image->ContentHash
	});
	overlay->Pixels = image->Pixels.data();
	overlay->Presentation = Make_Cursor_Presentation_Snapshot(
		front_end ? (int)MOUSE_NORMAL : (int)_SemanticMouseType, _CurrentShape,
		STATIC_POINTER_FRAME, STATIC_POINTER_HOT_X, STATIC_POINTER_HOT_Y, _CacheScale,
		image->Width, image->Height, image->ContentHash,
		_ContentGeneration.Current(), x, y);
	Trace_Cursor_Presentation(overlay->Presentation);
	return(true);
#else
	(void)overlay;
	return(false);
#endif
}


bool Win_Cursor_Is_Dirty(void)
{
#ifdef OPENTS_IOS
	WinCursorOverlay overlay;
	if (!Win_Cursor_Get_Overlay(&overlay)) {
		return(_OverlayDirty || _PresentedSnapshotValid);
	}

	return(_OverlayDirty || _ContentGeneration.Needs_Present() || !_PresentedSnapshotValid
		|| !(overlay.Presentation == _PresentedSnapshot));
#else
	return(false);
#endif
}


void Win_Cursor_Acknowledge_Present(WinCursorOverlay const & submitted_overlay)
{
#ifdef OPENTS_IOS
	WinCursorOverlay current_overlay;
	if (!Win_Cursor_Get_Overlay(&current_overlay)
		|| !(current_overlay.Presentation == submitted_overlay.Presentation)
		|| !_ContentGeneration.Acknowledge(submitted_overlay.Presentation.ContentGeneration)) {
		return;
	}

	_PresentedSnapshot = submitted_overlay.Presentation;
	_PresentedSnapshotValid = true;
	_OverlayDirty = false;
#else
	(void)submitted_overlay;
#endif
}


void Win_Cursor_Acknowledge_No_Overlay_Present(void)
{
#ifdef OPENTS_IOS
	WinCursorOverlay overlay;
	if (!Win_Cursor_Get_Overlay(&overlay)) {
		_PresentedSnapshotValid = false;
		_OverlayDirty = false;
	}
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
	_StaticPointerImage = CursorImage();
	_StaticPointerShape = NULL;
	_StaticPointerScale = 0;
	_PresentedSnapshotValid = false;
	_DiagnosticSnapshotValid = false;
}
