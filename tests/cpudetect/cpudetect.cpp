/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

// Checks Windows CPUID behavior and the Apple ARM64 CPU diagnostic contract. Needs no game
// data.

#if defined(_WIN32)
#include <windows.h>
#include <intrin.h>
#endif

#include <climits>
#include <cstdio>
#include <cstring>

#include "getcpu.h"
#include "mpu.h"

namespace {

int Failures = 0;


void Check(bool condition, char const * what)
{
	std::printf("%-52s %s\n", what, condition ? "ok" : "FAILED");

	if (!condition) {
		Failures++;
	}
}


#if defined(_WIN32)
/*
 * The detection reads the base family field only, as the assembly did. An extended family is
 * deliberately not folded in, so this reference computes the value the same narrow way.
 */
int Reference_Family(void)
{
	int regs[4];
	__cpuid(regs, 1);
	return((regs[0] & 0x0F00) >> 8);
}
#endif


void Check_Timing_Profiles(void)
{
	Check(Adjust_To_CPU_Timing_Profile(123, CpuTimingProfile::Neutral, 0, 0) == 123,
		"Neutral timing preserves positive input");
	Check(Adjust_To_CPU_Timing_Profile(0, CpuTimingProfile::Neutral, 0, 0) == 0,
		"Neutral timing preserves zero input");
	Check(Adjust_To_CPU_Timing_Profile(INT_MIN, CpuTimingProfile::Neutral, 0, 0) == INT_MIN,
		"Neutral timing preserves boundary input");
	Check(Adjust_To_CPU_Timing_Profile(333, CpuTimingProfile::LegacyX86PreP6, 20000000, 0) == 3330,
		"Pre-P6 timing matches legacy formula");
	Check(Adjust_To_CPU_Timing_Profile(333, CpuTimingProfile::LegacyX86P6OrLater, 20000000, 0) == 2660,
		"P6 timing preserves legacy truncation");
}


}	// namespace


int main(void)
{
	Check_Timing_Profiles();

#if defined(_WIN32)
	int regs[4];
	__cpuid(regs, 0);

	char vendor[16];
	std::memcpy(&vendor[0], &regs[1], 4);
	std::memcpy(&vendor[4], &regs[3], 4);
	std::memcpy(&vendor[8], &regs[2], 4);
	vendor[12] = '\0';

	int const maxleaf = regs[0];
	Check(maxleaf >= 1, "CPUID reports leaf 1");

	int const family = Reference_Family();

	std::printf("Reported vendor '%s', family %d\n\n", vendor, family);

	int cpu_type = -1;
	char reported[64];
	std::memset(reported, 0, sizeof(reported));

	Get_CPU_Type(cpu_type, reported, sizeof(reported) - 1);

	Check(cpu_type == family, "Get_CPU_Type family matches CPUID");
	Check(CPUType == (char)family, "CPUType global matches CPUID");

	/*
	 * CPU_Id writes the twelve vendor characters and then a space, so the buffer
	 * Get_CPU_Type copies out is the vendor followed by that separator.
	 */
	char expected[16];
	std::memcpy(expected, vendor, 12);
	expected[12] = ' ';
	expected[13] = '\0';
	Check(std::strcmp(reported, expected) == 0, "Vendor string matches CPUID");

	LARGE_INTEGER frequency = {};
	bool const has_frequency = QueryPerformanceFrequency(&frequency) != 0;
	Check(has_frequency, "QueryPerformanceFrequency is available");
	unsigned int rate_high = 0;
	unsigned int const rate_low = Get_CPU_Rate(rate_high);
	unsigned long long const rate = ((unsigned long long)rate_high << 32) | rate_low;
	if (has_frequency) {
		Check(rate == (unsigned long long)frequency.QuadPart, "Get_CPU_Rate matches QueryPerformanceFrequency");
	}
#elif defined(__APPLE__) && (defined(__aarch64__) || defined(__arm64__))
	int cpu_type = 0;
	char reported[64];
	std::memset(reported, 0, sizeof(reported));
	Get_CPU_Type(cpu_type, reported, sizeof(reported) - 1);

	Check(cpu_type == CPU_UNKNOWN, "Get_CPU_Type reports CPU_UNKNOWN on Apple ARM64");
	Check((unsigned char)CPUType == 0xFF, "CPUType raw byte is 0xFF on Apple ARM64");
	Check(std::strcmp(reported, "Apple ARM64") == 0, "Vendor string is Apple ARM64");
	Check(std::strcmp(VendorID, "Apple ARM64") == 0, "VendorID is Apple ARM64");
#else
#error "CpuDetect is only defined for Windows and Apple ARM64 targets."
#endif

	unsigned int high1 = 0;
	unsigned int const low1 = Get_CPU_Clock(high1);
	unsigned int high2 = 0;
	unsigned int const low2 = Get_CPU_Clock(high2);

	unsigned long long const stamp1 = ((unsigned long long)high1 << 32) | low1;
	unsigned long long const stamp2 = ((unsigned long long)high2 << 32) | low2;

	Check(stamp1 != 0, "Get_CPU_Clock returns a running count");
	Check(stamp2 >= stamp1, "Get_CPU_Clock advances");

	std::printf("\n%s\n", Failures == 0 ? "All checks passed." : "Some checks FAILED.");
	return(Failures == 0 ? 0 : 1);
}
