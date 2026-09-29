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

#include "always.h"

#include "movie.h"

#include "_keyboar.h"
#include "_map.h"
#include "_rect.h"
#include "_surface.h"
#include "ccfile.h"
#include "conquer.h"
#include "dbgprint.h"
#include "dsurface.h"
#include "globals.h"
#include "goptions.h"
#include "gscreen.h"
#include "movies.h"
#include "movieskip.h"
#include "legacyinputpolicy.hh"
#include "session.h"
#include "theme.h"
#include "vector.h"
#include "vqa.h"
#include "video.h"
#include "win.h"
#include "winstub.h"

#include "vq.hh"

#include <memory>
#include <string>


/// <summary>
/// Completes a half filled interpolated palette table.
/// The interpolation table is symmetric, so only one triangle of it needs to be stored on
/// disk. Use this routine to mirror the stored half across the diagonal after loading.
/// </summary>
/// <param name="interpal">Pointer to the 256 by 256 interpolation table to complete.</param>
void Rebuild_Interpolated_Palette(unsigned char * interpal)
{
	for (int y=0; y<SIZE_OF_PALETTE - 1; y++) {
		for (int x=y+1; x<SIZE_OF_PALETTE; x++) {
			*(interpal + (y*SIZE_OF_PALETTE+x)) = *(interpal + (x*SIZE_OF_PALETTE+y));
		}
	}
}


unsigned char 	* InterpolatedPalettes[100];
BOOL				PalettesRead;
unsigned			PaletteCounter;


struct TitleMovieSession::Impl
{
	enum class Phase : std::uint8_t { Idle, Playing, Complete, Fault };
	Phase State = Phase::Idle;
	std::string Name;
	std::unique_ptr<VQHandle> Handle;
	std::unique_ptr<MovieSkip::Playback> Playback;
	bool ClrScreenAfter = false;
	bool Available = false;
	bool MouseHidden = false;
	bool PlaybackBegun = false;
	bool NaturalCompletion = false;
	bool Skipped = false;
	bool CleanupDone = false;
};


TitleMovieSession::TitleMovieSession(void) : m_Impl(std::make_unique<Impl>())
{
}


TitleMovieSession::~TitleMovieSession(void)
{
	Finish();
}


bool TitleMovieSession::Begin(
	char const * name,
	ThemeType theme,
	bool clrscrn_after,
	bool stretch,
	bool clrscrn_before)
{
	if (m_Impl->State != Impl::Phase::Idle || name == NULL) return(false);
	if (Session.Type != GAME_NORMAL && !Session.PlayMovies) {
		m_Impl->State = Impl::Phase::Complete;
		return(true);
	}

	m_Impl->Name = name;
	m_Impl->ClrScreenAfter = clrscrn_after;
	m_Impl->Playback = std::make_unique<MovieSkip::Playback>(m_Impl->Name.c_str());
	if (!CCFileClass(m_Impl->Name.c_str()).Is_Available()) {
		m_Impl->State = Impl::Phase::Complete;
		return(true);
	}
	m_Impl->Available = true;
	Keyboard->Clear();

	m_Impl->Handle.reset(Movie_Create(
		m_Impl->Name.c_str(), HiddenSurface, Rect(0,0,0,0), Rect(0,0,0,0), 255, true));
	if (m_Impl->Handle == nullptr || m_Impl->Handle->VQA == nullptr) {
		m_Impl->State = Impl::Phase::Fault;
		return(false);
	}

	if (m_Impl->Handle->VQA->Get_VQA_Width() < 320 && m_Impl->Handle->VQA->Get_VQA_Height() < 200) {
		m_Impl->State = Impl::Phase::Fault;
		return(false);
	}

	bool const do_stretch = stretch && Options.StretchMovies;
	if (DSurface::AllowStretchBlits && do_stretch && m_Impl->Handle->InitialRect.Is_Valid()) {
		double const scale_x = (double)VisibleRect.Width / (double)m_Impl->Handle->InitialRect.Width;
		double const scale_y = (double)VisibleRect.Height / (double)m_Impl->Handle->InitialRect.Height;
		double const scale = scale_x < scale_y ? scale_x : scale_y;
		m_Impl->Handle->StretchRect.Width = (int)(m_Impl->Handle->InitialRect.Width * scale);
		m_Impl->Handle->StretchRect.Height = (int)(m_Impl->Handle->InitialRect.Height * scale);
		m_Impl->Handle->StretchRect.X = (VisibleRect.Width - m_Impl->Handle->StretchRect.Width) / 2;
		m_Impl->Handle->StretchRect.Y = (VisibleRect.Height - m_Impl->Handle->StretchRect.Height) / 2;
	}

	if (clrscrn_before || m_Impl->Handle->StretchRect != VisibleRect) {
		HiddenSurface->Fill(0);
		Update_Visible_Surface(HiddenSurface);
	}
	if (Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) {
		m_Impl->Handle->VQA->Set_Pause_On_Focus_Loss(false);
	}
	if (!Movie_Begin_Cooperative_Playback(m_Impl->Handle.get())) {
		m_Impl->State = Impl::Phase::Fault;
		return(false);
	}
	m_Impl->PlaybackBegun = true;
	Hide_Mouse();
	m_Impl->MouseHidden = true;
	if (theme != THEME_NONE) Theme.Queue_Song(theme);
	m_Impl->State = Impl::Phase::Playing;
	return(true);
}


TitleMovieStepResult TitleMovieSession::Step(void)
{
	using Result = TitleMovieStepResult;
	if (m_Impl->State == Impl::Phase::Complete) return(Result::Complete);
	if (m_Impl->State != Impl::Phase::Playing) return(Result::Fault);

	Call_Back();
	MSEngine_Service_Legacy_Input(
		[]() { Win_Gamepad_Service(); },
		[]() { Win_Gamepad_Discard_Actions(); },
		[]() { VQA_Message_Handler(); });
	if (MovieSkip::Idle()) {
		m_Impl->Skipped = true;
		Finish();
		m_Impl->State = Impl::Phase::Complete;
		return(Result::Complete);
	}

	bool finished = false;
	Movie_Advance_Cooperative_Frame(m_Impl->Handle.get(), finished);
	if (finished) {
		m_Impl->NaturalCompletion = true;
		Finish();
		m_Impl->State = Impl::Phase::Complete;
		return(Result::Complete);
	}
	return(Result::Waiting);
}


bool TitleMovieSession::Finish(void)
{
	if (m_Impl->CleanupDone) return(true);
	if (m_Impl->PlaybackBegun) {
		Movie_End_Cooperative_Playback(m_Impl->Handle.get());
		m_Impl->PlaybackBegun = false;
	}
	if (m_Impl->MouseHidden) {
		Show_Mouse();
		m_Impl->MouseHidden = false;
	}
	if (m_Impl->Handle != nullptr) {
		Movie_Destroy(m_Impl->Handle.get());
		m_Impl->Handle.reset();
		if (m_Impl->ClrScreenAfter) {
			HiddenSurface->Fill(0);
			Update_Visible_Surface(HiddenSurface);
		}
		Map.Flag_To_Redraw(GS_REDRAW_ALL);
		Keyboard->Clear();
	}
	m_Impl->Playback.reset();
	m_Impl->CleanupDone = true;
	return(true);
}


bool TitleMovieSession::Was_Available(void) const { return(m_Impl->Available); }
bool TitleMovieSession::Finished_Naturally(void) const { return(m_Impl->NaturalCompletion); }
bool TitleMovieSession::Was_Skipped(void) const { return(m_Impl->Skipped); }


/***********************************************************************************************
 * Play_Movie -- Plays a VQ movie.                                                             *
 *                                                                                             *
 *    Use this routine to play a VQ movie. It will dispatch the specified movie to the         *
 *    VQ player. The routine will not return until the movie has finished playing.             *
 *                                                                                             *
 * INPUT:   name  -- The name of the movie file (sans ".VQA").                                 *
 *                                                                                             *
 *          theme -- The identifier for an optional theme that should be played in the         *
 *                   background while this VQ plays.                                           *
 *                                                                                             *
 *          clrscrn -- 'true' if to clear the screen when the movie is over                    *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/19/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void Play_Movie(char const * name, ThemeType theme, bool clrscrn_after, bool stretch, bool clrscrn_before)
{
	// Outside a campaign, movies play only when the launch file asked for them.
	if (Session.Type != GAME_NORMAL && !Session.PlayMovies) {
		return;
	}

	// Opened ahead of the file test so the other machines count this movie even if it is missing here.
	MovieSkip::Playback playback(name);

	if (!CCFileClass(name).Is_Available()) {
		return;
	}

	Keyboard->Clear();

	VQHandle * vqa = Movie_Create(name, HiddenSurface, Rect(0,0,0,0), Rect(0,0,0,0), 255, 1);

	if (vqa != NULL) {

		if (vqa->VQA->Get_VQA_Width() < 320 && vqa->VQA->Get_VQA_Height() < 200) {
			Movie_Destroy(vqa);
			delete vqa;
			return;
		}

		bool dostretch = (stretch == true && Options.StretchMovies == true);

		if (DSurface::AllowStretchBlits == true && dostretch == true && vqa->InitialRect.Is_Valid()) {
			double scalex = (double)VisibleRect.Width / (double)vqa->InitialRect.Width;
			double scaley = (double)VisibleRect.Height / (double)vqa->InitialRect.Height;
			double scale = (scalex < scaley) ? scalex : scaley;

			vqa->StretchRect.Width = (int)(vqa->InitialRect.Width * scale);
			vqa->StretchRect.Height = (int)(vqa->InitialRect.Height * scale);
			vqa->StretchRect.X = (VisibleRect.Width - vqa->StretchRect.Width) / 2;
			vqa->StretchRect.Y = (VisibleRect.Height - vqa->StretchRect.Height) / 2;
			DebugString("Stretching movie %dx%d -> %dx%d\n", vqa->InitialRect.Width, vqa->InitialRect.Height, vqa->StretchRect.Width, vqa->StretchRect.Height);
		}

		/*
		**	Prepare to play a movie. First hide the mouse and stop any score that is playing.
		**	While the score (if any) is fading to silence, fade the palette to black as well.
		**	When the palette has finished fading, wait until the score has finished fading
		**	before launching the movie.
		*/
		// The screen around a movie that does not cover the display has to be cleared too.
		if (clrscrn_before || vqa->StretchRect != VisibleRect) {
			HiddenSurface->Fill(0);
			Update_Visible_Surface(HiddenSurface);
		}

		// The other machines wait for this one, so the movie runs on without the window's focus.
		if (Session.Type == GAME_IPX || Session.Type == GAME_INTERNET) {
			vqa->VQA->Set_Pause_On_Focus_Loss(false);
		}

		Movie_Play(vqa, true, theme, false);
		Movie_Destroy(vqa);
		delete vqa;

		/*
		**	Presume that the screen is left in a garbage state as well as the palette
		**	being in an unknown condition. Recover from this by clearing the screen and
		**	forcing the palette to black.
		*/
		if (clrscrn_after == true) {
			HiddenSurface->Fill(0);
			Update_Visible_Surface(HiddenSurface);
		}

		Map.Flag_To_Redraw(GS_REDRAW_ALL);
		Keyboard->Clear();
	}
}


/// <summary>
/// Plays a VQ movie with none of the usual trimmings.
/// This is the plain variant of Play_Movie -- the screen is blanked, the movie runs at the
/// default volume, and no stretching is attempted. A missing movie is quietly ignored.
/// </summary>
/// <param name="name">The name of the movie file, including the ".VQA" extension.</param>
/// <param name="theme">The optional theme to play behind the movie.</param>
void _Play_Movie(char const * name, ThemeType theme)
{
	bool notavailable = CCFileClass(name).Is_Available() == false;
	if (!notavailable) {
		HiddenSurface->Fill(0);
		Update_Visible_Surface(HiddenSurface);
		Keyboard->Clear();
		VQHandle * vqa = Movie_Create(name, HiddenSurface, Rect(0,0,0,0), Rect(0,0,0,0), 255, true);
		if (vqa != NULL) {
			Movie_Play(vqa, true, theme, true);
			Movie_Destroy(vqa);
			delete vqa;
			Keyboard->Clear();
		}
	}
}


/// <summary>
/// Plays a VQ movie.
/// This routine is the convenient form that takes a movie identifier rather than a filename.
/// A movie identifier of VQ_NONE is quietly ignored.
/// </summary>
void Play_Movie(VQType vq, ThemeType theme, bool clrscrn, bool stretch)
{
	static char _buf[MOVIE_FILENAME_SIZE];
	if (vq != VQ_NONE) {
		snprintf(_buf, sizeof(_buf), "%s.VQA", Movies[vq]);
		Play_Movie(_buf, theme, clrscrn, stretch, true);
	}
}


/// <summary>
/// Plays a movie on the sidebar.
/// This routine queues the movie up to play within the sidebar surface while the game
/// carries on around it. A missing movie is quietly ignored, and so is a game outside a
/// campaign unless the launch file asked for movies.
/// </summary>
/// <param name="name">The name of the movie file, including the ".VQA" extension.</param>
void Play_Ingame_Movie(const char * name)
{
	bool notavailable = CCFileClass(name).Is_Available() == false;
	if (!notavailable && (Session.Type == GAME_NORMAL || Session.PlayMovies)) {
		VQHandle * vqa = Movie_Create(name, SidebarSurface, Rect(0,0,0,0), SidebarSurface->Get_Rect(), 255, false);
		if (vqa != NULL) {
			Movie_Queue_Ingame(vqa);
		}
	}
}


/// <summary>
/// Plays a movie on the sidebar.
/// This routine is the convenient form that takes a movie identifier rather than a filename.
/// A movie identifier of VQ_NONE is quietly ignored.
/// </summary>
void Play_Ingame_Movie(VQType vq)
{
	static char _buf[MOVIE_FILENAME_SIZE];
	if (vq != VQ_NONE) {
		snprintf(_buf, sizeof(_buf), "%s.VQA", Movies[vq]);
		Play_Ingame_Movie(_buf);
	}
}


/// <summary>
/// Shuts down any in game movies that are playing.
/// This routine tears down every queued sidebar movie and releases it. Call it whenever the
/// sidebar is going away or the scenario is ending.
/// </summary>
void Stop_Ingame_Movie(void)
{
	for (int i = 0; i < IngameVQ.Count(); i++) {
		VQHandle * vqa = IngameVQ[i];
		if (vqa != NULL) {
			Movie_Destroy(vqa);
			delete vqa;
		}
	}

	IngameVQ.Clear();
}


/// <summary>
/// Suspends or resumes the in game movie.
/// Use this routine when the game itself is being suspended, so that the sidebar movie does
/// not run on while everything else is stopped.
/// </summary>
/// <param name="pause">Should the movie be suspended rather than resumed?</param>
void Pause_Ingame_Movie(bool pause)
{
	if (Has_Ingame_Movies()) {
		VQHandle * vqa = IngameVQ[0];
		if (pause == true) {
			Movie_Pause(vqa);
			DebugString("IngameVQ Pause\n");
		} else {
			Movie_Resume(vqa);
			DebugString("IngameVQ Resume\n");
		}
	}
}


/// <summary>
/// Is there an in game movie playing on the sidebar?
/// </summary>
/// <returns>bool; Is at least one in game movie queued up?</returns>
bool Has_Ingame_Movies(void)
{
	return(IngameVQ.Count() > 0);
}
