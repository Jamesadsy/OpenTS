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

// A process-relative, monotonic millisecond reading. It is for elapsed runtime
// and presentation work only; save/load must rebase values derived from it.
std::int64_t Monotonic_Milliseconds(void);

// The historical common-call-site width, retaining its natural unsigned wrap.
unsigned int System_Milliseconds(void);

#if defined(OPENTS_MONOTONIC_TEST)
// Test-only seam. Production builds always read std::chrono::steady_clock.
using Monotonic_Test_Clock = std::int64_t (*)(void);
void Monotonic_Set_Test_Clock(Monotonic_Test_Clock clock);
void Monotonic_Reset_Test_Clock(void);
#endif
