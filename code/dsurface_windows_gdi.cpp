/*******************************************************************************
 *                                O P E N  T S
 ******************************************************************************/

#include "always.h"

#ifdef _WIN32

#include "dsurface.h"
#include "dsurface_windows_gdi.h"
#include "video.h"

class DSurfaceWindowsGDIBackend
{
public:
	HBITMAP Bitmap = NULL;
	HDC DC = NULL;
	HGDIOBJ OldBitmap = NULL;
};

bool DSurfaceWindowsGDIAdapter::Initialize(DSurface & surface, int width, int height)
{
	if (width <= 0 || height <= 0) {
		return(false);
	}

	DSurfaceWindowsGDIBackend * backend = new DSurfaceWindowsGDIBackend;
	struct {
		BITMAPINFOHEADER Header;
		unsigned long Masks[3];
	} info = {};

	info.Header.biSize = sizeof(BITMAPINFOHEADER);
	info.Header.biWidth = width;
	info.Header.biHeight = -height;
	info.Header.biPlanes = 1;
	info.Header.biBitCount = 16;
	info.Header.biCompression = BI_BITFIELDS;
	info.Masks[0] = 0xF800;
	info.Masks[1] = 0x07E0;
	info.Masks[2] = 0x001F;

	backend->DC = CreateCompatibleDC(NULL);
	if (backend->DC == NULL) {
		delete backend;
		return(false);
	}

	backend->Bitmap = CreateDIBSection(backend->DC, (BITMAPINFO *)&info, DIB_RGB_COLORS,
		&surface.PixelBuffer, NULL, 0);
	if (backend->Bitmap == NULL) {
		DeleteDC(backend->DC);
		delete backend;
		surface.PixelBuffer = NULL;
		return(false);
	}

	backend->OldBitmap = SelectObject(backend->DC, backend->Bitmap);
	DIBSECTION section;
	surface.Pitch = width * 2;
	if (GetObject(backend->Bitmap, sizeof(section), &section) == sizeof(section)) {
		surface.Pitch = section.dsBm.bmWidthBytes;
	}
	surface.WindowsGDIBackend = backend;
	return(true);
}

void DSurfaceWindowsGDIAdapter::Shutdown(DSurface & surface)
{
	DSurfaceWindowsGDIBackend * backend = surface.WindowsGDIBackend;
	if (backend == NULL) {
		return;
	}
	if (backend->DC != NULL) {
		if (backend->OldBitmap != NULL) {
			SelectObject(backend->DC, backend->OldBitmap);
		}
		DeleteDC(backend->DC);
	}
	if (backend->Bitmap != NULL) {
		DeleteObject(backend->Bitmap);
	}
	delete backend;
	surface.WindowsGDIBackend = NULL;
	surface.PixelBuffer = NULL;
	surface.Pitch = 0;
}

HDC DSurfaceWindowsGDIAdapter::Acquire(DSurface & surface)
{
	DSurfaceWindowsGDIBackend * backend = surface.WindowsGDIBackend;
	if (backend == NULL || backend->DC == NULL) {
		return(NULL);
	}
	++surface.LockCount;
	return(backend->DC);
}

int DSurfaceWindowsGDIAdapter::Release(DSurface & surface, HDC)
{
	GdiFlush();
	if (surface.LockCount > 0) {
		--surface.LockCount;
	}
	if (surface.IsPrimary && surface.LockCount == 0) {
		Video_Mark_Dirty();
	}
	return(1);
}

bool DSurfaceWindowsGDIAdapter::Stretch_Blit(DSurface & destination, DSurface const & source,
	int dest_x, int dest_y, int dest_width, int dest_height,
	int source_x, int source_y, int source_width, int source_height)
{
	DSurfaceWindowsGDIBackend * dest = destination.WindowsGDIBackend;
	DSurfaceWindowsGDIBackend * src = source.WindowsGDIBackend;
	if (dest == NULL || src == NULL || dest->DC == NULL || src->DC == NULL) {
		return(false);
	}
	GdiFlush();
	SetStretchBltMode(dest->DC, COLORONCOLOR);
	bool result = StretchBlt(dest->DC, dest_x, dest_y, dest_width, dest_height,
		src->DC, source_x, source_y, source_width, source_height, SRCCOPY) != 0;
	GdiFlush();
	return(result);
}

#endif
