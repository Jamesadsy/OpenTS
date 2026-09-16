/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * Contains material derived from Electronic Arts source code.
 * Modified by OpenTS contributors, 2026.
 * EA's GPLv3 Section 7 additional terms and supplemental warranty
 * disclaimers apply; see LICENSE.md.
 ******************************************************************************/

#include "dbgprint.h"

#include <algorithm>
#include <chrono>
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <mutex>

namespace
{

constexpr std::size_t DEBUG_MESSAGE_MAX = 4096;

// Apple G2 diagnostics deliberately own no persistent path.  Keeping this as one static
// object makes both path queries stable and explicit without introducing a directory service.
char const No_Debug_Path[] = "";
char const No_Win32_Error_Text[] = "Apple diagnostic error text is unavailable";

// A recursive lock is intentional: a same-thread diagnostic re-entry must not self-deadlock,
// while calls from different threads still serialize complete logical records.
std::recursive_mutex DebugLock;
bool AtLineStart = true;

void Write_Text_Locked(char const * text, std::size_t length)
{
	if (length == 0) {
		return;
	}

	std::fwrite(text, 1, length, stderr);
	std::fflush(stderr);
}


void Write_Message_Locked(char const * buffer, bool with_prefix)
{
	std::size_t const length = std::strlen(buffer);
	if (length == 0) {
		return;
	}

	// The prefix identifies a logical record rather than a function call.  As in the Windows
	// oracle, a continuation call therefore reaches the same line without another timestamp.
	if (with_prefix && AtLineStart) {
		auto const now = std::chrono::system_clock::now();
		auto const whole_seconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
		long long milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
			now - whole_seconds).count();
		if (milliseconds < 0) {
			milliseconds += 1000;
		}

		std::time_t const wall_clock = std::chrono::system_clock::to_time_t(whole_seconds);
		std::tm local_time = {};
		::localtime_r(&wall_clock, &local_time);

		char stamped[DEBUG_MESSAGE_MAX + 32];
		int const written = std::snprintf(stamped, sizeof(stamped),
			"[%02d:%02d:%02d.%03lld] %s",
			local_time.tm_hour, local_time.tm_min, local_time.tm_sec, milliseconds, buffer);
		if (written > 0) {
			std::size_t const kept = std::min<std::size_t>(
				static_cast<std::size_t>(written), sizeof(stamped) - 1);
			Write_Text_Locked(stamped, kept);
			AtLineStart = buffer[length - 1] == '\n';
			return;
		}
	}

	Write_Text_Locked(buffer, length);
	AtLineStart = buffer[length - 1] == '\n';
}


void Emit(char const * buffer, bool with_prefix)
{
	std::lock_guard<std::recursive_mutex> lock(DebugLock);
	Write_Message_Locked(buffer, with_prefix);
}

} // namespace


void Debug_Init(void)
{
	// Apple G2 diagnostics use the already-owned stderr stream and have no initialization that
	// can create a window, event owner, file, directory or host service.
}


void Debug_Init_Console(void)
{
	// There is no Win32 console to emulate.  The accepted Apple diagnostic sink is stderr.
}


void Debug_Console_Hold(void)
{
	// Apple must never block waiting for console input.
}


char const * Debug_Log_File_Name(void)
{
	return(No_Debug_Path);
}


char const * Debug_Directory(void)
{
	return(No_Debug_Path);
}


bool Delete_Files_Older_Than(char const * directory, char const * pattern, unsigned days)
{
	// The Apple G2 profile owns no diagnostic files, so pruning fails closed for every input.
	(void)directory;
	(void)pattern;
	(void)days;
	return(false);
}


void __cdecl DebugString(char const * string, ...)
{
	int const saved_errno = errno;

	if (string != nullptr) {
		char buffer[DEBUG_MESSAGE_MAX];
		va_list va;
		va_start(va, string);
		std::vsnprintf(buffer, sizeof(buffer), string, va);
		va_end(va);
		Emit(buffer, true);
	}

	errno = saved_errno;
}


void __cdecl DebugStringNoPrefix(char const * string, ...)
{
	int const saved_errno = errno;

	if (string != nullptr) {
		char buffer[DEBUG_MESSAGE_MAX];
		va_list va;
		va_start(va, string);
		std::vsnprintf(buffer, sizeof(buffer), string, va);
		va_end(va);
		Emit(buffer, false);
	}

	errno = saved_errno;
}


char const * Last_Error_Text(unsigned long error)
{
	(void)error;
	return(No_Win32_Error_Text);
}
