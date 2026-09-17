/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2025 Electronic Arts Inc.
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/wwlib/mpu.h                                  $*
 *                                                                                             *
 *                      $Author:: Denzil_l                                                    $*
 *                                                                                             *
 *                     $Modtime:: 8/17/01 6:11p                                               $*
 *                                                                                             *
 *                    $Revision:: 3                                                           $*
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#pragma once


enum class CpuTimingProfile {
	Neutral,
	LegacyX86PreP6,
	LegacyX86P6OrLater,
};

// Get_CPU_Clock returns a diagnostic counter. It is RDTSC on Windows and a process-relative
// steady-clock count on Apple ARM64.
extern "C" {
	unsigned int __cdecl Get_CPU_Clock(unsigned int & high);
}

#if defined(_WIN32)
unsigned int Get_CPU_Rate(unsigned int & high);

void RDTSC(void);
int Get_RDTSC_CPU_Speed(void);
#endif

int Adjust_To_CPU_Timing_Profile(int time, CpuTimingProfile profile, unsigned int counter_low,
	unsigned int counter_high);
