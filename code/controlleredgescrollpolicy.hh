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


enum class PointerEdgeScrollSource {
	Suppressed,
	NativeImmediate,
	ControllerDelayed
};


inline PointerEdgeScrollSource Pointer_Edge_Scroll_Source(bool controller_owns_pointer,
	bool direct_touch, bool right_stick_pan)
{
	if (direct_touch || right_stick_pan) {
		return(PointerEdgeScrollSource::Suppressed);
	}

	return(controller_owns_pointer ? PointerEdgeScrollSource::ControllerDelayed
		: PointerEdgeScrollSource::NativeImmediate);
}


class ControllerEdgeScrollDwell
{
	public:
		static constexpr uint64_t DWELL_MS = 2000;

		bool Should_Scroll(int direction, uint64_t now_ms)
		{
			if (direction == 0) {
				Reset();
				return(false);
			}

			if (!_HasDirection || direction != _Direction || now_ms < _EnteredAtMs) {
				_Direction = direction;
				_EnteredAtMs = now_ms;
				_HasDirection = true;
				return(false);
			}

			return(now_ms - _EnteredAtMs >= DWELL_MS);
		}

		void Reset(void)
		{
			_Direction = 0;
			_EnteredAtMs = 0;
			_HasDirection = false;
		}

	private:
		int _Direction = 0;
		uint64_t _EnteredAtMs = 0;
		bool _HasDirection = false;
};
