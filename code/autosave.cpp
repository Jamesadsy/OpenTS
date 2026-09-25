/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "autosave.h"
#include "sha.h"

#include <cctype>
#include <cstdio>
#include <cstring>


void AutosaveClass::Set_Interval(int frames)
{
	IntervalFrames = frames > 0 ? frames : 0;
}


void AutosaveClass::Seed_Slots(ProductType product, int campaign, int skirmish)
{
	std::array<int, 2> & slots = NextSlots[Product_Index(product)];
	slots[0] = Held_Slot(campaign);
	slots[1] = Held_Slot(skirmish);
}


int AutosaveClass::Campaign_Slot(ProductType product) const
{
	return(NextSlots[Product_Index(product)][0]);
}


int AutosaveClass::Skirmish_Slot(ProductType product) const
{
	return(NextSlots[Product_Index(product)][1]);
}


void AutosaveClass::Schedule(int frame)
{
	NextFrame = IntervalFrames > 0 ? frame + IntervalFrames : -1;
	IsArmed = false;
}


bool AutosaveClass::Is_Due(int frame) const
{
	return(!IsArmed && NextFrame >= 0 && frame >= NextFrame);
}


void AutosaveClass::Arm(void)
{
	IsArmed = true;
}


bool AutosaveClass::Take_Armed(void)
{
	bool armed = IsArmed;
	IsArmed = false;
	return(armed);
}


/// <summary>
/// The file an automatic save uses for a source scenario identity.
/// </summary>
std::string AutosaveClass::File_Name(KindType kind, std::string_view scenario_identity)
{
	std::string normalized;
	normalized.reserve(scenario_identity.size());
	for (unsigned char character : scenario_identity) {
		if (character >= 'a' && character <= 'z') {
			character = static_cast<unsigned char>(character - 'a' + 'A');
		}
		if (character == '\\') {
			character = '/';
		}
		normalized.push_back(static_cast<char>(character));
	}
	if (normalized.empty()) {
		normalized = "UNKNOWN_SCENARIO";
	}

	SHAEngine sha;
	sha.Hash(normalized.data(), static_cast<int>(normalized.size()));
	unsigned char digest[20];
	sha.Result(digest);

	char const * prefix = kind == KindType::Campaign ? "AUTOSAVE_" : "AUTOSAVE_SKIRMISH_";
	std::string name(prefix);
	for (unsigned char byte : digest) {
		char encoded[3];
		std::snprintf(encoded, sizeof(encoded), "%02X", static_cast<unsigned int>(byte));
		name += encoded;
	}
	name += ".SAV";

	return(name);
}


int AutosaveClass::Held_Slot(int slot)
{
	return(slot >= 0 && slot < SLOT_COUNT ? slot : 0);
}


std::size_t AutosaveClass::Product_Index(ProductType product)
{
	return(product == ProductType::Firestorm ? 1 : 0);
}


std::string Quick_Save_File_Name(AutosaveClass::KindType kind)
{
	return(kind == AutosaveClass::KindType::Campaign ? "QUICKSAVE.SAV" : "QUICKSAVE_SKIRMISH.SAV");
}


std::string Multiplayer_Save_File_Name(int slot)
{
	char name[24];
	std::snprintf(name, sizeof(name), "SVGM_%03d.NET", slot);
	return(name);
}


int Multiplayer_Save_Slot(char const * file_name)
{
	if (file_name == NULL || std::strlen(file_name) != 12 || _strnicmp(file_name, "SVGM_", 5) != 0
		|| _stricmp(file_name + 8, ".NET") != 0) {
		return(-1);
	}

	int slot = 0;
	for (int index = 5; index < 8; index++) {
		if (!std::isdigit(static_cast<unsigned char>(file_name[index]))) {
			return(-1);
		}
		slot = slot * 10 + (file_name[index] - '0');
	}
	return(slot);
}
