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
#include <string>
#include <string_view>

/*
 * Automatic-save timing and legacy slot metadata. File names are derived from the active
 * scenario identity, so the stored slot values only preserve the existing save layout.
 */
class AutosaveClass
{
	public:

		// Legacy save data carries one slot value for each product and game kind.
		static constexpr int SLOT_COUNT = 5;

		enum class KindType {
			Campaign,
			Skirmish,
		};

		// Tiberian Sun and Firestorm save separate legacy slot values.
		enum class ProductType {
			TiberianSun,
			Firestorm,
		};

		// An interval of zero or less turns automatic saves off.
		void Set_Interval(int frames);
		int Interval(void) const {return(IntervalFrames);}

		// Reads and writes the legacy next-slot values carried in existing saves and launch files.
		void Seed_Slots(ProductType product, int campaign, int skirmish);
		int Campaign_Slot(ProductType product) const;
		int Skirmish_Slot(ProductType product) const;

		// Any completed save schedules from the frame it was written on, and drops an armed request.
		void Schedule(int frame);
		bool Is_Due(int frame) const;
		void Arm(void);
		bool Take_Armed(void);

		// Stable for the source scenario filename held by ScenarioName.
		static std::string File_Name(KindType kind, std::string_view scenario_identity);

	private:

		static int Held_Slot(int slot);
		static std::size_t Product_Index(ProductType product);

		int IntervalFrames = 0;
		int NextFrame = -1;
		bool IsArmed = false;
		std::array<std::array<int, 2>, 2> NextSlots = {};
};

// The file a quick save of one kind of game is written under and read back from.
std::string Quick_Save_File_Name(AutosaveClass::KindType kind);

// Multiplayer saves are numbered from zero in the pattern the client lists, a thousand at most.
constexpr int MULTIPLAYER_SAVE_SLOTS = 1000;
std::string Multiplayer_Save_File_Name(int slot);

// The slot a numbered multiplayer save's file name carries, or -1 for any other name.
int Multiplayer_Save_Slot(char const * file_name);
