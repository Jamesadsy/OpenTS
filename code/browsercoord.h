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

namespace OpenTS_Browser
{

enum class CooperativeResult : int
{
	Active = 0,
	Waiting = 1,
	Complete = 2,
	Fault = 3
};

enum class BrowserProcessPhase : int
{
	Idle = 0,
	ControlProbe = 1,
	Complete = 2,
	Fault = 3
};

inline constexpr std::uint32_t ControlProbeStepLimit = 300;

struct BrowserProcessSession
{
	BrowserProcessPhase Phase = BrowserProcessPhase::Idle;
	std::uint32_t StepCount = 0;
	bool BeginOwned = false;
	bool StepActive = false;
	bool FinishOwned = false;
	int FaultCode = 0;
};

bool Begin(BrowserProcessSession & session) noexcept;
CooperativeResult Step(BrowserProcessSession & session) noexcept;
BrowserProcessPhase Status(BrowserProcessSession const & session) noexcept;
bool Finish(BrowserProcessSession & session) noexcept;

} // namespace OpenTS_Browser

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#define OPENTS_BROWSER_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define OPENTS_BROWSER_EXPORT
#endif

extern "C"
{
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Begin(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Step(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Status(void);
OPENTS_BROWSER_EXPORT int OpenTS_Browser_Finish(void);
}
