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

/*
 * Bookkeeping for the rotating automatic saves. Frames are handed in, so it holds no engine
 * state and can be judged without the game running.
 */
class AutosaveClass
{
	public:

		// One ring per kind of game. The client reads a slot back as a single digit.
		static constexpr int SLOT_COUNT = 5;

		enum class KindType {
			Campaign,
			Skirmish,
		};

		// Tiberian Sun and Firestorm use separate save folders and keep separate rings.
		enum class ProductType {
			TiberianSun,
			Firestorm,
		};

		// An interval of zero or less turns automatic saves off.
		void Set_Interval(int frames);
		int Interval(void) const {return(IntervalFrames);}

		// Slots are counted from zero; one the ring does not hold starts the ring over.
		void Seed_Slots(ProductType product, int campaign, int skirmish);
		int Campaign_Slot(ProductType product) const;
		int Skirmish_Slot(ProductType product) const;

		// Any completed save schedules from the frame it was written on, and drops an armed request.
		void Schedule(int frame);
		bool Is_Due(int frame) const;
		void Arm(void);
		bool Take_Armed(void);

		// Answers the slot to write and moves the ring on, so a save records the slot after it.
		int Advance(ProductType product, KindType kind);

		static std::string File_Name(KindType kind, int slot);

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
