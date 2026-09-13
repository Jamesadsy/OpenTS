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

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                     $Archive:: /G/wwlib/dsurface.h                                         $*
 *                                                                                             *
 *                      $Author:: Neal_k                                                      $*
 *                                                                                             *
 *                     $Modtime:: 6/23/00 2:24p                                               $*
 *                                                                                             *
 *                    $Revision:: 2                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once

#include "palette.h"
#include "xsurface.h"

#include <vector>

#ifdef _WIN32
class DSurfaceWindowsGDIBackend;
class DSurfaceWindowsGDIAdapter;
#endif


enum DSurfaceColorMode {
	COLORMODE_INVALID = -1,
	COLORMODE_555,
	COLORMODE_556,
	COLORMODE_565,
	COLORMODE_655,
};


// A concrete, CPU-addressable RGB565 surface.  Its portable contract is the pixel buffer,
// dimensions, stride, lock/unlock and software drawing operations below.  A Windows build may
// attach a private DIB/HDC accelerator, but no native drawing handle is part of this type's
// cross-platform API.
class DSurface : public XSurface
{
		typedef XSurface BASECLASS;

	public:
		virtual ~DSurface(void) override;

		/*
		**	Constructs a working surface (not visible).
		*/
		DSurface(int width, int height);

		/*
		**	Create a surface object that represents the currently visible screen.
		*/
		static DSurface * Create_Primary(void);

		/*
		**	Copies regions from one surface to another.
		*/
		virtual bool Blit_From(Rect const & dcliprect, Rect const & destrect, Surface const & source, Rect const & scliprect, Rect const & sourcerect, bool trans=false, bool unknown=true) override;
		virtual bool Blit_From(Rect const & destrect, Surface const & source, Rect const & sourcerect, bool trans=false, bool unknown=true) override;
		virtual bool Blit_From(Surface const & source, bool trans=false, bool unknown=true) override {return(BASECLASS::Blit_From(source, trans, unknown));}

		/*
		**	Fills a region with a constant color.
		*/
		virtual bool Fill_Rect(Rect const & rect, int color) override;
		virtual bool Fill_Rect(Rect const & cliprect, Rect const & fillrect, int color) override;
		virtual bool Fill(int color) override;
		virtual bool Fill_Rect_Trans(Rect const & xcliprect, RGBClass const & color, unsigned int opacity) override;

		virtual bool Draw_Depth_Shaded_Line(Rect const & cliprect, Point2D const & startpoint, Point2D const & endpoint, unsigned color, int start_depth, int end_depth, bool write_depth = false) override;
		virtual bool Draw_Depth_Glow_Line(Rect const & cliprect, Point2D const & startpoint, Point2D const & endpoint, int glow_strength, int start_depth, int end_depth, bool write_depth = false) override;
		virtual bool Draw_Depth_Antialiased_Line(Rect const & cliprect, Point2D const & startpoint, Point2D const & endpoint, RGBClass & color, int start_depth, int end_depth, bool write_depth, bool blend_red, bool blend_green, bool blend_blue, float intensity) override;

		virtual int Draw_Masked_Dashed_Line(Point2D const & startpoint, Point2D const & endpoint, unsigned color, bool pattern[], int offset, bool draw_on_zero_alpha) override;
		virtual bool Draw_Masked_Line(Point2D const & startpoint, Point2D const & endpoint, unsigned color, bool draw_on_zero_alpha) override;
		virtual bool Draw_Ping_Pong_Gradient_Line(Rect const & cliprect, Point2D const & startpoint, Point2D const & endpoint, RGBClass const & start_color, RGBClass const & end_color, float & gradient_step, float & gradient_position) const;

		/*
		**	Gets and frees a direct pointer to the surface memory.
		*/
		virtual void * Lock(Point2D point = Point2D(0, 0)) const override;
		virtual bool Unlock(void) const override;
		virtual bool Can_Lock(int x = 0, int y = 0) const override;

		/*
		 * The pixels, reachable without the lock bookkeeping. The presenter reads the
		 * frame this way, since a locked surface refuses to be blitted from.
		 */
		void * Get_Buffer(void) const {return(PixelBuffer);}

		/*
		**	Queries information about the surface.
		*/
		virtual int Bytes_Per_Pixel(void) const override;
		virtual int Stride(void) const override;

		virtual bool Can_Blit(void) const;

		/*
		 * The movie player scales to the full screen only when this is true. Surfaces
		 * stretch in software now, so it always is.
		 */
		static bool AllowStretchBlits;

		/*
		 * The bit layout that the primary surface packs its color guns into. The
		 * surfaces are always 565, so it never changes.
		 */
		static int PrimaryColorMode;

		static int Build_Hicolor_Pixel(int red, int green, int blue);
		static int Build_Hicolor_Pixel(RGBClass const & rgb);
		static RGBClass Deconstruct_Hicolor_Pixel(unsigned short pixel);
		static unsigned short Blend_Pixel(unsigned short src_color, unsigned short dst_color, int level);
		static void Build_Remap_Table(unsigned short * table, int count, PaletteClass const & palette);
		static unsigned short Get_Halfbright_Mask(void) {return(HalfbrightMask);}
		static unsigned short Get_Quarterbright_Mask(void) {return(QuarterbrightMask);}
		static unsigned short Get_Eighthbright_Mask(void) {return(EighthbrightMask);}

		static int Get_Red_Right(void);
		static int Get_Red_Left(void);
		static int Get_Green_Right(void);
		static int Get_Green_Left(void);
		static int Get_Blue_Right(void);
		static int Get_Blue_Left(void);
		static int Get_Primary_Color_Mode(void);

	protected:

		/*
		**	Convenient copy of the bytes per pixel value to speed accessing it. It
		**	gets accessed frequently.
		*/
		mutable int BytesPerPixel;

		// Set for the frame consumed by Video_Present.  Any completed mutation of this
		// surface marks that frame dirty, independently of the host presentation API.
		bool IsPrimary;

		// Apple and other non-Windows hosts own ordinary CPU memory here.  Windows can
		// leave it empty while its private backend exposes the DIB's CPU pixels through
		// PixelBuffer.  RGB565 rows are naturally four-byte aligned (two bytes/pixel).
		std::vector<unsigned char> PixelStorage;
		void * PixelBuffer;
		int Pitch;

#ifdef _WIN32
		// Deliberately opaque to the portable surface implementation and absent on Apple.
		DSurfaceWindowsGDIBackend * WindowsGDIBackend;
		friend class DSurfaceWindowsGDIAdapter;
#endif

	public:
		/*
		**	Shift values to extract the gun value from a hicolor pixel such that the
		**	gun component is normalized to a byte value.
		*/
		static int RedRight;
		static int RedLeft;
		static int BlueRight;
		static int BlueLeft;
		static int GreenRight;
		static int GreenLeft;

	protected:
		static unsigned short HalfbrightMask;
		static unsigned short QuarterbrightMask;
		static unsigned short EighthbrightMask;

	private:
		/*
		**	This prevents the creation of a surface in ways that are not
		**	supported.
		*/
		DSurface(DSurface const & rvalue);
		DSurface const operator = (DSurface const & rvalue);
};


/***********************************************************************************************
 * DSurface::Build_Hicolor_Pixel -- Construct a hicolor pixel according to the surface pixel f *
 *                                                                                             *
 *    This routine will construct a pixel according to the highcolor pixel format for this     *
 *    surface.                                                                                 *
 *                                                                                             *
 * INPUT:   red   -- The red component of the color (0..255).                                  *
 *                                                                                             *
 *          green -- The green component of the color (0..255).                                *
 *                                                                                             *
 *          blue  -- The blue component of the color (0..255).                                 *
 *                                                                                             *
 * OUTPUT:  Returns with a screen format pixel number that most closesly matches the color     *
 *          specified.                                                                         *
 *                                                                                             *
 * WARNINGS:   The return value is card dependant and only applies to hicolor displays.        *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   05/27/1997 JLB : Created.                                                                 *
 *=============================================================================================*/
inline int DSurface::Build_Hicolor_Pixel(int red, int green, int blue)
{
	return(((red >> RedLeft) << RedRight) | ((green >> GreenLeft) << GreenRight) | ((blue >> BlueLeft) << BlueRight));
}


inline int DSurface::Build_Hicolor_Pixel(RGBClass const & rgb)
{
	return(((rgb.Get_Red() >> RedLeft) << RedRight) | ((rgb.Get_Green() >> GreenLeft) << GreenRight) | ((rgb.Get_Blue() >> BlueLeft) << BlueRight));
}


inline RGBClass DSurface::Deconstruct_Hicolor_Pixel(unsigned short color)
{
	return(RGBClass(
		color >> DSurface::RedRight << DSurface::RedLeft,
		color >> DSurface::GreenRight << DSurface::GreenLeft,
		color >> DSurface::BlueRight << DSurface::BlueLeft));
}
