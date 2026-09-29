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

#pragma once

#include "theme.hh"
#include "vq.hh"

#include <cstdint>
#include <memory>

template<class T> class DynamicVectorClass;

extern DynamicVectorClass<char const *> Movies;

// Room for the longest name the movie registry accepts, its ".VQA" and the terminator.
// RulesClass::Do_Movies reads a name into a 32 byte buffer, so 31 characters can arrive.
inline constexpr int MOVIE_FILENAME_SIZE = 36;

void Play_Movie(char const * name, ThemeType theme=THEME_NONE, bool clrscrn_after=true, bool stretch=true, bool clrscrn_before=true);
void Play_Movie(VQType vq, ThemeType theme=THEME_NONE, bool clrscrn=true, bool stretch=true);
void Play_Ingame_Movie(VQType vq);
void Pause_Ingame_Movie(bool pause);
void Stop_Ingame_Movie(void);
bool Has_Ingame_Movies(void);

enum class TitleMovieStepResult : std::uint8_t {
	Waiting,
	Complete,
	Fault,
};

class TitleMovieSession
{
	public:
		TitleMovieSession(void);
		~TitleMovieSession(void);
		TitleMovieSession(TitleMovieSession const &) = delete;
		TitleMovieSession & operator=(TitleMovieSession const &) = delete;

		bool Begin(char const * name, ThemeType theme, bool clrscrn_after, bool stretch, bool clrscrn_before);
		TitleMovieStepResult Step(void);
		bool Finish(void);

		bool Was_Available(void) const;
		bool Finished_Naturally(void) const;
		bool Was_Skipped(void) const;

	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
};
