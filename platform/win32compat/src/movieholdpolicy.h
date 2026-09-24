/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <cstdint>

class MovieSkipHold
{
	public:
		enum class Source { Touch, Circle };
		static constexpr uint64_t HOLD_NS = 500ULL * 1000000ULL;

		void Press(Source source, uint64_t now)
		{
			State & state = Slot(source);
			if (!state.Down) {
				state.Down = true;
				state.Start = now;
			}
		}

		void Release(Source source)
		{
			Slot(source) = {};
		}

		bool Ready(uint64_t now)
		{
			if (Fired) {
				return(false);
			}
			if (Mature(Touch, now) || Mature(Circle, now)) {
				Fired = true;
				return(true);
			}
			return(false);
		}

		void Reset(void)
		{
			Touch = {};
			Circle = {};
			Fired = false;
		}

	private:
		struct State { bool Down = false; uint64_t Start = 0; };
		State Touch;
		State Circle;
		bool Fired = false;

		State & Slot(Source source)
		{
			return(source == Source::Touch ? Touch : Circle);
		}

		static bool Mature(State const & state, uint64_t now)
		{
			return(state.Down && now >= state.Start && now - state.Start >= HOLD_NS);
		}
};
