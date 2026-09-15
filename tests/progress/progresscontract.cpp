/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "progress.h"

#include <cmath>
#include <iostream>


namespace {

int Failures = 0;


void Check(char const * name, bool condition)
{
	if (!condition) {
		std::cerr << name << " failed\n";
		Failures++;
	}
}


void Check_Close(char const * name, double actual, double expected)
{
	Check(name, std::abs(actual - expected) < 0.000001);
}


void Test_Player_Count(void)
{
	Check("zero players become one", ProgressScreenContract::Player_Count(0) == 1);
	Check("negative players become one", ProgressScreenContract::Player_Count(-1) == 1);
	Check("player count is retained", ProgressScreenContract::Player_Count(4) == 4);
}


void Test_Single_Player_Progress(void)
{
	Check_Close("percent maps to job units", ProgressScreenContract::Progress_From_Percent(400.0, 12.5), 50.0);
	Check_Close("single player fraction", ProgressScreenContract::Player_Fraction(25.0, 100.0), 0.25);
	Check_Close("single player average", ProgressScreenContract::Average_Fraction(100.0, 1, 100.0), 1.0);
	Check_Close("multi-player average", ProgressScreenContract::Average_Fraction(100.0, 2, 100.0), 0.5);
}


void Test_Progress_Clamp(void)
{
	Check_Close("progress clamps at job total", ProgressScreenContract::Clamp_To_Main_Progress(125.0, 100.0), 100.0);
	Check_Close("progress lower bound remains caller-owned", ProgressScreenContract::Clamp_To_Main_Progress(-5.0, 100.0), -5.0);
}

} // namespace


int main(void)
{
	Test_Player_Count();
	Test_Single_Player_Progress();
	Test_Progress_Clamp();

	if (Failures != 0) {
		std::cerr << Failures << " progress-screen contract checks failed\n";
		return(1);
	}

	std::cout << "All progress-screen contract checks passed\n";
	return(0);
}
