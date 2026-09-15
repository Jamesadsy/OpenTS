/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Keep this include first: dict.h must be usable without win.h and must not manufacture the
// historical Windows truth macros.
#include "dict.h"

#ifdef TRUE
#error "dict.h must not introduce TRUE"
#endif
#ifdef FALSE
#error "dict.h must not introduce FALSE"
#endif

#include "productprofile.h"
#include "session.h"

#include <cstdio>
#include <type_traits>


namespace
{

int Failures = 0;
int Checks = 0;

void Check(char const * name, bool condition)
{
	Checks++;
	if (!condition) {
		std::printf("FAIL %s\n", name);
		Failures++;
	}
}

unsigned int Int_Hash(int & value)
{
	return(static_cast<unsigned int>(value));
}

bool Has_Deferred_Source(std::string_view name)
{
	return(OpenTSProductProfile::Is_Deferred_Source(name));
}

void Check_Profile(void)
{
	using namespace OpenTSProductProfile;

#if defined(OPENTS_APPLE_SINGLE_PLAYER_PROFILE)
	Check("profile name is Apple Single-Player", Name == "Apple Single-Player");
	Check("Apple profile selected", Is_Apple_Single_Player && !Is_Windows_Full);
	Check("Apple retains Session shared state", Session_Shared_State_Retained);
	Check("Apple retains campaign state", Campaign_State_Retained);
	Check("Apple retains portable Save Browser", Portable_Save_Browser_Retained);
	Check("Apple retains portable Game Controls", Portable_Game_Controls_Retained);
	Check("Apple LAN route is unavailable", !Route_Available(MenuRoute::Lan));
	Check("Apple Internet route is unavailable", !Route_Available(MenuRoute::Internet));
	Check("Apple skirmish route is unavailable", !Route_Available(MenuRoute::Skirmish));
	Check("Apple deferral inventory is nonempty", !Deferred_Source_Families.empty());
	Check("netdlg2 source is deferred", Has_Deferred_Source("netdlg2.cpp"));
	Check("network lobby source is deferred", Has_Deferred_Source("netshare.cpp"));
	Check("Win32 skirmish source is deferred", Has_Deferred_Source("skirmish.cpp"));
#else
	Check("profile name is Windows Full", Name == "Windows Full");
	Check("Windows full profile selected", Is_Windows_Full && !Is_Apple_Single_Player);
	Check("Windows retains Session shared state", Session_Shared_State_Retained);
	Check("Windows retains campaign state", Campaign_State_Retained);
	Check("Windows retains portable Save Browser", Portable_Save_Browser_Retained);
	Check("Windows retains portable Game Controls", Portable_Game_Controls_Retained);
	Check("Windows LAN route is available", Route_Available(MenuRoute::Lan));
	Check("Windows Internet route is available", Route_Available(MenuRoute::Internet));
	Check("Windows skirmish route is available", Route_Available(MenuRoute::Skirmish));
	Check("Windows has no deferred source inventory", Deferred_Source_Families.empty());
	Check("Windows keeps netdlg2 source", !Has_Deferred_Source("netdlg2.cpp"));
#endif
}

void Check_Dictionary_Contract(void)
{
	Dictionary<int, int> dictionary(Int_Hash);
	int first_key = 7;
	int first_value = 11;
	int second_key = 19;
	int second_value = 23;
	int replacement = 29;
	int value = 0;

	Check("dictionary add returns historical char success", dictionary.add(first_key, first_value) != 0);
	Check("dictionary second add returns historical char success", dictionary.add(second_key, second_value) != 0);
	Check("dictionary contains returns historical char success", dictionary.contains(first_key) != 0);
	Check("dictionary getValue preserves storage contract", dictionary.getValue(first_key, value) != 0 && value == first_value);
	Check("dictionary update returns historical char success", dictionary.updateValue(first_key, replacement) != 0);
	Check("dictionary update preserves value", dictionary.getValue(first_key, value) != 0 && value == replacement);

	int index = 0;
	int offset = 0;
	int iterated = 0;
	while (dictionary.iterate(index, offset, value)) {
		iterated++;
	}
	Check("dictionary iterate returns stored entries", iterated == 2);
	Check("dictionary remove returns historical char success", dictionary.remove(second_key) != 0);
	Check("dictionary removed key is absent", dictionary.contains(second_key) == 0);
	Check("dictionary entry count remains truthful", dictionary.getEntries() == 1);
}

} // namespace


int main(void)
{
	static_assert(std::is_class_v<SessionClass>, "SessionClass must remain in the product profile");
	static_assert(GAME_NORMAL == 0, "GAME_NORMAL is the local single-player session type");

	Check_Profile();
	Check_Dictionary_Contract();

	std::printf("%d Apple/Windows profile contract checks: %s\n",
		Checks,
		Failures == 0 ? "all passed" : "failures");
	return(Failures == 0 ? 0 : 1);
}
