/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <array>
#include <string_view>

#if defined(OPENTS_APPLE_SINGLE_PLAYER_PROFILE) == defined(OPENTS_WINDOWS_FULL_PROFILE)
#error "Exactly one OpenTS product profile must be selected"
#endif

namespace OpenTSProductProfile
{

enum class MenuRoute {
	Lan,
	Internet,
	Skirmish,
};

#if defined(OPENTS_APPLE_SINGLE_PLAYER_PROFILE)

inline constexpr bool Is_Apple_Single_Player = true;
inline constexpr bool Is_Windows_Full = false;
inline constexpr std::string_view Name = "Apple Single-Player";
inline constexpr bool Session_Shared_State_Retained = true;
inline constexpr bool Campaign_State_Retained = true;
inline constexpr bool Portable_Save_Browser_Retained = true;
inline constexpr bool Portable_Game_Controls_Retained = true;

// This is the source-level inventory of capabilities deliberately left out of the Apple
// product target. It is kept beside the profile contract so the build and proof test cannot
// silently drift apart.
inline constexpr std::array<std::string_view, 10> Deferred_Source_Families = {
	"chat.cpp",
	"desync.cpp",
	"desyncdlg.cpp",
	"desyncdlg_stub.cpp",
	"_desyncdlg.cpp",
	"mplayer.cpp",
	"netdlg2.cpp",
	"netshare.cpp",
	"sendfile.cpp",
	"skirmish.cpp",
};

constexpr bool Route_Available(MenuRoute)
{
	return false;
}

#else

inline constexpr bool Is_Apple_Single_Player = false;
inline constexpr bool Is_Windows_Full = true;
inline constexpr std::string_view Name = "Windows Full";
inline constexpr bool Session_Shared_State_Retained = true;
inline constexpr bool Campaign_State_Retained = true;
inline constexpr bool Portable_Save_Browser_Retained = true;
inline constexpr bool Portable_Game_Controls_Retained = true;

inline constexpr std::array<std::string_view, 0> Deferred_Source_Families = {};

constexpr bool Route_Available(MenuRoute)
{
	return true;
}

#endif

constexpr bool Is_Deferred_Source(std::string_view source)
{
	for (std::string_view deferred : Deferred_Source_Families) {
		if (deferred == source) {
			return true;
		}
	}
	return false;
}

} // namespace OpenTSProductProfile
