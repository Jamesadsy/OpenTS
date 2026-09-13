/*******************************************************************************
 *                                O P E N  T S
 ******************************************************************************/

#pragma once

// This adapter is intentionally Windows-only.  DSurface itself never publishes an HDC,
// HBITMAP or HGDIOBJ; genuine Win32 UI consumers opt in by including this header.
#ifdef _WIN32

#include <windows.h>

class DSurface;

class DSurfaceWindowsGDIAdapter
{
public:
	static bool Initialize(DSurface & surface, int width, int height);
	static void Shutdown(DSurface & surface);
	static HDC Acquire(DSurface & surface);
	static int Release(DSurface & surface, HDC hdc);
	static bool Stretch_Blit(DSurface & destination, DSurface const & source,
		int dest_x, int dest_y, int dest_width, int dest_height,
		int source_x, int source_y, int source_width, int source_height);
};

#endif
