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

/* $Header: /counterstrike/EGOS.CPP 2     3/10/97 3:19p Steve_tall $ */
/*************************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S                  **
 *************************************************************************************
 *                                                                                   *
 *                 Project Name : Command & Conquer - Red Alert                      *
 *                                                                                   *
 *                    File Name : EGOS.CPP                                           *
 *                                                                                   *
 *                   Programmer : Steve Tall                                         *
 *                                                                                   *
 *                   Start Date : September 4th, 1996                                *
 *                                                                                   *
 *                  Last Update : September 4th, 1996 [ST]                           *
 *                                                                                   *
 *-----------------------------------------------------------------------------------*
 * Overview:                                                                         *
 *                                                                                   *
 *   Scrolling movie style credits.                                                  *
 *                                                                                   *
 *-----------------------------------------------------------------------------------*
 * Functions:                                                                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "always.h"

#include "egos.h"

#include "_keyboar.h"
#include "_palette.h"
#include "_rect.h"
#include "_surface.h"
#include "_timer.h"
#include "addon.h"
#include "ccfile.h"
#include "conquer.h"
#include "dialog.h"
#include "dsurface.h"
#include "font.h"
#include "gadget.h"
#include "globals.h"
#include "goptions.h"
#include "gscreen.h"
#include "hostclock.h"
#include "language/language.h"
#include "misc.h"
#include "drawhelp.h"
#include "scheme.h"
#include "theme.h"
#include "utf8.h"
#include "video.h"
#include "vector.h"

#include "color.hh"
#include "dialog.hh"

#include <algorithm>
#include <cstring>

/*
**	List of Ego Class instances
**	There will be one instance for each line of text.
*/
DynamicVectorClass<EgoClass *> EgoList;

/*
**	Number of slideshow pictures
*/

#define NUM_SLIDES 17

/*
**	Length of time frame is displayed for
*/
#define FRAME_DELAY	150

/*
**	Number of frames that palete fade occurs over
*/
#define FADE_DELAY		37

/*
**	Original copy of slide (pref in video mem) that we use to undraw the text
*/
Surface *BackgroundSurface;


/***********************************************************************************************
 * EC::EgoClass -- EgoClass constructor                                                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    x position of text                                                                *
 *           y position of text                                                                *
 *           ptr to text string                                                                *
 *           flags to print text with                                                          *
 *                                                                                             *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    9/9/96 11:53PM ST : Created                                                              *
 *=============================================================================================*/
EgoClass::EgoClass (int x, int y, char *text, TextPrintType flags)
{
	XPos = x;
	YPos = y;
	Flags= flags;
	Text = new char [strlen (text)+1];
	strcpy (Text, text);
}


/***********************************************************************************************
 * EC::~EgoClass -- EgoClass destructor                                                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    9/9/96 11:54PM ST : Created                                                              *
 *=============================================================================================*/
EgoClass::~EgoClass(void)
{
	delete [] Text;
}


/***********************************************************************************************
 * EC::Scroll -- Apply the given distance to the y position of the text.                       *
 *               A positive distance scrolls up.                                               *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    distance in pixels to scroll up                                                   *
 *                                                                                             *
 * OUTPUT:   true if text scrolled beyond the top of the screen                                *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    9/9/96 11:55PM ST : Created                                                              *
 *=============================================================================================*/
bool EgoClass::Scroll(int distance)
{
	YPos -= distance;
	if (YPos < -20) {
		return(true);
	}else{
		return(false);
	}
}


/***********************************************************************************************
 * EC::Render -- Draws the text to the logic page                                              *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    9/9/96 11:57PM ST : Created                                                              *
 *=============================================================================================*/
void EgoClass::Render(bool fresh)
{
	if ((YPos < LogicalSurface->Get_Height() && YPos > LogicalSurface->Get_Height() - 52) || YPos >= -16 && YPos <= 32 || fresh) {
		static HFONT font;
		if (font == NULL) {
			HDC dc = GetDC(MainWindow);
			font = WS_Get_Font(dc, "Arial", 0, 16, 1);
			ReleaseDC(MainWindow, dc);
		}

		Rect textrect(XPos, YPos, VideoModeWidth, 0);

		int alignment = 0;
		if (Flags & TPF_CENTER) {
			alignment = OD_TEXT_ALIGN_CENTER;
		}else{
			if (Flags & TPF_RIGHT){
				alignment = OD_TEXT_ALIGN_MAX;
			}
		}

		if (GameInFocus) {
			OD_Draw_Text(RGB(255, 255, 128), font, textrect, Text, strlen(Text), alignment, 0, BackgroundSurface);
		}
	}
}


/***********************************************************************************************
 * EC::Wipe -- Wipes the previously rendered text by blitting a rectangle from the given       *
 *             background screen.                                                              *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to screen containing original background                                      *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    9/9/96 11:58PM ST : Created                                                              *
 *=============================================================================================*/
void EgoClass::Wipe (Surface *background)
{
	int width = Font_From_TPF(Flags)->String_Pixel_Width (Text);
	int x = XPos;

	if (Flags & TPF_RIGHT) {
		x -= width;
	}else{
		if (Flags & TPF_CENTER){
			x -= width/2;
		}
	}

	Rect r(0, YPos, VideoModeWidth, 18);
	LogicalSurface->Blit_From(r, *background, r);
}


/***********************************************************************************************
 * CreditsSession -- Retained owner for the native scrolling credits presentation.            *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    9/10/96 0:20AM ST : Created                                                              *
 *=============================================================================================*/
struct CreditsSession::Impl
{
	enum class Phase : std::uint8_t { Idle, Scroll, Fade, Complete, Fault };
	Phase State = Phase::Idle;
	std::unique_ptr<char[]> Text;
	std::unique_ptr<DSurface> Background;
	float OldVolume = 0.0f;
	int Time = 0;
	int Frame = 0;
	int PictureFrame = 0;
	int SlideNumber = 0;
	int Speed = 2;
	bool Fresh = false;
	bool ResourcesReady = false;
	bool CleanupDone = false;
};


CreditsSession::CreditsSession(void) : m_Impl(std::make_unique<Impl>())
{
}


CreditsSession::~CreditsSession(void)
{
	Finish();
}


bool CreditsSession::Begin(void)
{
	using Phase = Impl::Phase;
	if (m_Impl->State != Phase::Idle) return(false);
	if (Addon_Enabled(ADDON_FIRESTORM)) {
		m_Impl->State = Phase::Complete;
		return(true);
	}

	CCFileClass creditsfile("TSCREDIT.txt");
	if (!creditsfile.Is_Available()) {
		m_Impl->State = Phase::Complete;
		return(true);
	}

	int const text_length = creditsfile.Size();
	m_Impl->Text = std::make_unique<char[]>((size_t)text_length + 1);
	creditsfile.Read(m_Impl->Text.get(), text_length);
	int length = text_length;
	int line = 0;
	int column = 0;
	char * cptr = m_Impl->Text.get();
	char ch = 0;
	char lastchar = 0;
	char oldchar = 0;
	char * strstart = nullptr;
	char * strparse = nullptr;
	bool gotendstr = false;
	int startcolumn = 0;
	int endcolumn = 0;
	int x = 0;
	int y = HiddenSurface->Get_Height() + 2;
	int lastline = -2;
	int const bom = (int)UTF8::BOM_Length(std::string_view(cptr, length));
	cptr += bom;
	length -= bom;

	LogicalSurface = AlternateSurface;
	GadgetClass::Set_Color_Scheme("Green");
	Fancy_Text_Print(TXT_NONE, *LogicalSurface, LogicalSurface->Get_Rect(), Point2D(0, 0),
		Fetch_Scheme_By_Name("LightGold"), TBLACK,
		TextPrintType(TPF_CENTER | TPF_METAL12 | TPF_FULLSHADOW));

	if (length > 0) do {
		ch = *cptr++;
		length--;
		switch (ch) {
			case 13:
				line++;
				y += 16;
				column = 0;
				break;
			case 10:
				break;
			case 32:
				column++;
				break;
			case 9:
				column += 8;
				column &= 0xfffffff8;
				break;
			default: {
				lastchar = ch;
				strstart = cptr - 1;
				strparse = cptr - 1;
				endcolumn = startcolumn = column;
				gotendstr = false;
				do {
					ch = *strparse++;
					switch (ch) {
						case 9:
						case 10:
						case 13: gotendstr = true; break;
						case 32: if (lastchar == 32) gotendstr = true; break;
						default: break;
					}
					if (strparse >= cptr + length) gotendstr = true;
					lastchar = ch;
				} while (!gotendstr);
				if (strparse >= cptr + length) break;
				if (*(strparse - 2) == 32) strparse--;

				TextPrintType flags = TextPrintType(TPF_METAL12 | TPF_FULLSHADOW);
				if (startcolumn > 3 && endcolumn < 8) {
					flags = TextPrintType(flags | TPF_CENTER);
					x = HiddenSurface->Get_Width() / 2;
				} else if (startcolumn < 3) {
					flags = TextPrintType(flags | TPF_RIGHT);
					x = (HiddenSurface->Get_Width() / 2) - (HiddenSurface->Get_Width() / 12);
				} else {
					x = HiddenSurface->Get_Width() / 2;
				}
				if (startcolumn > 3 && endcolumn < 8 && line - lastline > 1) {
					flags = TextPrintType(flags | TPF_LIGHTSHADOW);
				} else {
					flags = TextPrintType(flags | TPF_DROPSHADOW);
				}
				lastline = line;
				oldchar = *(strparse - 1);
				*(strparse - 1) = 0;
				EgoList.Add(new EgoClass(x, y, strstart, flags));
				*(strparse - 1) = oldchar;
				cptr = strparse;
				column += (int)(strparse - strstart);
				length -= (int)(strparse - strstart) - 1;
				if (ch == 13) {
					line++;
					y += 16;
					column = 0;
				} else if (ch == 9) {
					column += 7;
					column &= 0xfffffff8;
				}
				break;
			}
		}
	} while (length > 0);

	Theme.Stop();
	HiddenSurface->Fill(TBLACK);
	Update_Visible_Surface();
	m_Impl->Background = std::make_unique<DSurface>(VisibleRect.Width, VisibleRect.Height);
	BackgroundSurface = m_Impl->Background.get();
	m_Impl->Background->Blit_From(*HiddenSurface);
	Keyboard->Clear();
	LogicalSurface = HiddenSurface;
	m_Impl->OldVolume = Options.ScoreVolume;
	if ((double)m_Impl->OldVolume == 0) Options.Set_Score_Volume(0.4f, false);
	Theme.Queue_Song(Theme.From_Name("MADRAP"));
	m_Impl->Time = TickCount;
	Hide_Mouse();
	m_Impl->ResourcesReady = true;
	m_Impl->State = Phase::Scroll;
	return(true);
}


CreditsStepResult CreditsSession::Step(void)
{
	using Phase = Impl::Phase;
	using Result = CreditsStepResult;
	if (m_Impl->State == Phase::Complete) return(Result::Complete);
	if (m_Impl->State != Phase::Scroll && m_Impl->State != Phase::Fade) return(Result::Fault);

	Call_Back();
	if (m_Impl->State == Phase::Scroll && Keyboard->Check()) {
		if (Keyboard->Get() == KN_ESC) {
			Theme.Fade_Out();
			m_Impl->State = Phase::Complete;
			return(Finish() ? Result::Complete : Result::Fault);
		}
	}
	if (m_Impl->Frame != 0
		&& TickCount - m_Impl->Time < m_Impl->Frame * m_Impl->Speed) {
		return(Result::Waiting);
	}

	if (m_Impl->State == Phase::Scroll) {
		m_Impl->Frame++;
		m_Impl->PictureFrame++;
		if (m_Impl->PictureFrame > FRAME_DELAY + 50) {
			if (m_Impl->SlideNumber < NUM_SLIDES - 1) {
				m_Impl->SlideNumber++;
				m_Impl->PictureFrame = 0;
			}
		}
		for (int i = EgoList.Count() - 1; i >= 0; i--) {
			if (EgoList[i]->Scroll(2)) {
				delete EgoList[i];
				EgoList.Delete_Index(i);
				break;
			}
		}

		Rect scroll_dest(0, 32, VideoModeWidth, VideoModeHeight - 66);
		Rect scroll_source(0, 34, VideoModeWidth, VideoModeHeight - 66);
		Rect top_band(0, 0, VideoModeWidth, 34);
		Rect bottom_band(0, VideoModeHeight - 34, VideoModeWidth, 34);
		BackgroundSurface->Fill_Rect(top_band, TBLACK);
		BackgroundSurface->Fill_Rect(bottom_band, TBLACK);
		for (int i = EgoList.Count() - 1; i >= 0; i--) EgoList[i]->Render(m_Impl->Fresh);

		unsigned short * bsurf = (unsigned short *)BackgroundSurface->Lock();
		if (bsurf != nullptr) {
			int const step = BackgroundSurface->Stride() / 2;
			int fade_y = 0;
			for (int raw_alpha = 256; raw_alpha > 0; raw_alpha -= 8) {
				int const alpha = std::min(raw_alpha, 255);
				int const xidx = step * fade_y;
				int const yidx = step * (VideoModeHeight - fade_y - 1);
				for (int fade_x = 0; fade_x < VideoModeWidth; fade_x++) {
					(bsurf + xidx)[fade_x] = OD_Blend_Color((bsurf + xidx)[fade_x], 0, alpha);
					(bsurf + yidx)[fade_x] = OD_Blend_Color((bsurf + yidx)[fade_x], 0, alpha);
				}
				fade_y++;
			}
			BackgroundSurface->Unlock();
		}
		if (GameInFocus) {
			if (m_Impl->Fresh) {
				VisibleSurface->Blit_From(*BackgroundSurface);
			} else {
				VisibleSurface->Blit_From(top_band, *BackgroundSurface, top_band);
				VisibleSurface->Blit_From(scroll_dest, *VisibleSurface, scroll_source);
				VisibleSurface->Blit_From(bottom_band, *BackgroundSurface, bottom_band);
			}
		}
		m_Impl->Fresh = false;
		if (m_Impl->Frame > 1000 && !Theme.Still_Playing()) Theme.Queue_Song(THEME_NONE);
		if (EgoList.Count() == 0) m_Impl->State = Phase::Fade;
		return(Result::Waiting);
	}

	if (m_Impl->PictureFrame < FRAME_DELAY && m_Impl->PictureFrame > 10 + FADE_DELAY) {
		m_Impl->PictureFrame = FRAME_DELAY;
	}
	m_Impl->Frame++;
	m_Impl->PictureFrame++;
	if (m_Impl->PictureFrame > FADE_DELAY + FRAME_DELAY) {
		m_Impl->State = Phase::Complete;
		return(Finish() ? Result::Complete : Result::Fault);
	}
	return(Result::Waiting);
}


bool CreditsSession::Finish(void)
{
	if (m_Impl->CleanupDone) return(true);
	if (m_Impl->ResourcesReady) {
		HiddenSurface->Fill(TBLACK);
		Update_Visible_Surface();
		Show_Mouse();
		GadgetClass::Set_Color_Scheme("Blue");
		Theme.Stop();
		Options.Set_Score_Volume(m_Impl->OldVolume, false);
		BackgroundSurface = nullptr;
		m_Impl->Background.reset();
		while (EgoList.Count() > 0) {
			delete EgoList[0];
			EgoList.Delete_Index(0);
		}
		EgoList.Clear();
		m_Impl->ResourcesReady = false;
	}
	m_Impl->Text.reset();
	m_Impl->CleanupDone = true;
	m_Impl->State = Impl::Phase::Complete;
	return(true);
}


void Show_Who_Was_Responsible(void)
{
	CreditsSession session;
	if (!session.Begin()) return;
	CreditsStepResult result = CreditsStepResult::Waiting;
	while (result == CreditsStepResult::Waiting) {
		result = session.Step();
		if (result == CreditsStepResult::Waiting) Host_Sleep(Video_Milliseconds_Until_Present());
	}
	session.Finish();
}
