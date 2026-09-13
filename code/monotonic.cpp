/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "monotonic.h"

#include <chrono>

namespace
{
	std::int64_t Steady_Clock_Milliseconds(void)
	{
		using namespace std::chrono;
		static steady_clock::time_point const process_origin = steady_clock::now();
		return(duration_cast<milliseconds>(steady_clock::now() - process_origin).count());
	}

#if defined(OPENTS_MONOTONIC_TEST)
	Monotonic_Test_Clock TestClock = nullptr;
#endif
}


std::int64_t Monotonic_Milliseconds(void)
{
#if defined(OPENTS_MONOTONIC_TEST)
	if (TestClock != nullptr) {
		return(TestClock());
	}
#endif
	return(Steady_Clock_Milliseconds());
}


unsigned int System_Milliseconds(void)
{
	return(static_cast<unsigned int>(Monotonic_Milliseconds()));
}


#if defined(OPENTS_MONOTONIC_TEST)
void Monotonic_Set_Test_Clock(Monotonic_Test_Clock clock)
{
	TestClock = clock;
}


void Monotonic_Reset_Test_Clock(void)
{
	TestClock = nullptr;
}
#endif
