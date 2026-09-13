/*******************************************************************************
 *                                O P E N  T S
 ******************************************************************************/

#include "dsurface.h"

#include "_zbuffer.h"
#include "abuffer.h"

#include <cstdio>

// The surface's primary notification is a presentation semantic.  The test observes it through
// this minimal host seam rather than needing a window, renderer or proprietary game data.
int VideoModeWidth = 0;
int VideoModeHeight = 0;
int DirtyNotifications = 0;
void Video_Mark_Dirty(void)
{
	++DirtyNotifications;
}

ABuffer * AlphaBuffer = NULL;

namespace {

int Failures = 0;
int Checks = 0;

void Check(bool condition, char const * name)
{
	++Checks;
	if (!condition) {
		++Failures;
		std::printf("FAIL: %s\n", name);
	}
}

void Set(DSurface & surface, int x, int y, unsigned short pixel)
{
	unsigned short * pixels = (unsigned short *)surface.Lock(Point2D(x, y));
	Check(pixels != NULL, "pixel lock succeeds");
	if (pixels != NULL) {
		*pixels = pixel;
		surface.Unlock();
	}
}

unsigned short Get(DSurface const & surface, int x, int y)
{
	unsigned short * pixels = (unsigned short *)surface.Lock(Point2D(x, y));
	Check(pixels != NULL, "pixel read lock succeeds");
	unsigned short pixel = pixels != NULL ? *pixels : 0;
	if (pixels != NULL) ((DSurface &)surface).Unlock();
	return(pixel);
}

void Test_Allocation_And_Locking()
{
	DSurface surface(5, 3);
	Check(surface.Get_Buffer() != NULL, "CPU RGB565 allocation");
	Check(surface.Get_Width() == 5 && surface.Get_Height() == 3, "surface dimensions");
	Check(surface.Bytes_Per_Pixel() == 2 && surface.Stride() >= 10 && surface.Stride() % 4 == 0, "RGB565 pitch");
	Check(surface.Lock(Point2D(-1, 0)) == NULL && surface.Lock(Point2D(5, 0)) == NULL, "out of range lock rejected");
	unsigned short * pixels = (unsigned short *)surface.Lock();
	Check(pixels != NULL && surface.Is_Locked(), "lock exposes mutable pixels");
	if (pixels != NULL) {
		pixels[0] = 0x1234;
		Check(surface.Unlock() && !surface.Is_Locked(), "unlock balances lock");
	}
	Check(Get(surface, 0, 0) == 0x1234, "locked mutation persists");
}

void Test_Fill_Copy_And_Transparency()
{
	DSurface source(2, 2);
	DSurface destination(4, 3);
	Check(destination.Fill_Rect(Rect(0, 0, 4, 3), 0x1CE7), "fill succeeds");
	Check(Get(destination, 3, 2) == 0x1CE7, "fill writes RGB565 pixel");

	Set(source, 0, 0, 0x1111);
	Set(source, 1, 0, 0x0000);
	Set(source, 0, 1, 0x3333);
	Set(source, 1, 1, 0x4444);
	Check(destination.Blit_From(Rect(1, 1, 2, 2), source, Rect(0, 0, 2, 2)), "same-size copy succeeds");
	Check(Get(destination, 1, 1) == 0x1111 && Get(destination, 2, 2) == 0x4444, "same-size copy pixels");
	destination.Fill(0x1CE7);
	Check(destination.Blit_From(Rect(1, 1, 2, 2), source, Rect(0, 0, 2, 2), true), "transparent copy succeeds");
	Check(Get(destination, 2, 1) == 0x1CE7, "transparent RGB565 zero is skipped");
}

void Test_Scaled_Copy_And_Clipping()
{
	DSurface source(2, 2);
	DSurface destination(4, 4);
	Set(source, 0, 0, 0x1001);
	Set(source, 1, 0, 0x2002);
	Set(source, 0, 1, 0x3003);
	Set(source, 1, 1, 0x4004);
	Check(destination.Blit_From(Rect(0, 0, 4, 4), source, Rect(0, 0, 2, 2)), "scaled copy succeeds");
	Check(Get(destination, 0, 0) == 0x1001 && Get(destination, 3, 0) == 0x2002
		&& Get(destination, 0, 3) == 0x3003 && Get(destination, 3, 3) == 0x4004, "nearest RGB565 scaling");

	destination.Fill(0x5555);
	Check(destination.Blit_From(Rect(-1, 1, 2, 2), source, Rect(0, 0, 2, 2)), "clipped copy succeeds");
	Check(Get(destination, 0, 1) == 0x2002 && Get(destination, 0, 2) == 0x4004, "clipping retains mapped source edge");
	Check(Get(destination, 1, 1) == 0x5555 && Get(destination, 0, 0) == 0x5555, "clipping leaves exterior unchanged");
}

void Test_Primary_Dirty_Notification()
{
	VideoModeWidth = 3;
	VideoModeHeight = 2;
	DirtyNotifications = 0;
	DSurface * primary = DSurface::Create_Primary();
	Check(primary != NULL, "primary CPU surface allocation");
	if (primary != NULL) {
		primary->Fill(0x07E0);
		Check(DirtyNotifications == 1, "primary fill marks presentation dirty");
		primary->Lock();
		primary->Unlock();
		Check(DirtyNotifications == 2, "primary unlock marks presentation dirty");
		delete primary;
	}
}

}

int main()
{
	Test_Allocation_And_Locking();
	Test_Fill_Copy_And_Transparency();
	Test_Scaled_Copy_And_Clipping();
	Test_Primary_Dirty_Notification();
	std::printf("DSurface: %d checks, %d failures\n", Checks, Failures);
	return(Failures == 0 ? 0 : 1);
}
