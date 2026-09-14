/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "savefileenum.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#endif


namespace
{


std::string Lowercase(std::string value)
{
	std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
		return((char)std::tolower(character));
	});
	return(value);
}


bool Extension_Matches(std::filesystem::path const & path, char const * requested)
{
	if (requested == nullptr || requested[0] == '\0') {
		return(false);
	}

	std::string wanted(requested);
	if (wanted[0] != '.') {
		wanted.insert(wanted.begin(), '.');
	}

	return(Lowercase(path.extension().string()) == Lowercase(wanted));
}


bool Is_Ineligible(std::filesystem::path const & path)
{
#ifdef _WIN32
	DWORD const attributes = GetFileAttributesA(path.string().c_str());
	if (attributes == INVALID_FILE_ATTRIBUTES) {
		return(true);
	}

	return((attributes & (FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_DIRECTORY |
		FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)) != 0);
#else
	/* POSIX has no equivalent attribute set. A dot-prefixed entry is its hidden-file rule. */
	std::string const name = path.filename().string();
	return(!name.empty() && name[0] == '.');
#endif
}


std::chrono::system_clock::time_point To_System_Clock(std::filesystem::file_time_type value)
{
	using FileClock = std::filesystem::file_time_type::clock;

	/*
	 * file_time_type is allowed to use a clock different from system_clock. Anchor both clocks
	 * at the instant of conversion and carry only the resulting portable duration forward.
	 */
	FileClock::time_point const file_now = FileClock::now();
	std::chrono::system_clock::time_point const system_now = std::chrono::system_clock::now();
	return(system_now + std::chrono::duration_cast<std::chrono::system_clock::duration>(value - file_now));
}


}


std::vector<SaveFileRecord> Enumerate_Save_Files(std::string const & directory, char const * extension)
{
	std::vector<SaveFileRecord> records;
	std::error_code error;
	std::filesystem::directory_iterator files(
		directory,
		std::filesystem::directory_options::skip_permission_denied,
		error);

	if (error) {
		return(records);
	}

	for (std::filesystem::directory_iterator end; files != end; files.increment(error)) {
		if (error) {
			error.clear();
			continue;
		}

		std::filesystem::path const path = files->path();
		if (!Extension_Matches(path, extension) || Is_Ineligible(path)) {
			continue;
		}

		error.clear();
		if (!files->is_regular_file(error) || error) {
			error.clear();
			continue;
		}

		error.clear();
		std::filesystem::file_time_type const modified = files->last_write_time(error);
		if (error) {
			error.clear();
			continue;
		}

		SaveFileRecord record;
		record.Filename = path.filename().string();
		record.ModifiedAt = To_System_Clock(modified);
		records.push_back(record);
	}

	std::sort(records.begin(), records.end(), [](SaveFileRecord const & first, SaveFileRecord const & second) {
		if (first.ModifiedAt != second.ModifiedAt) {
			return(first.ModifiedAt > second.ModifiedAt);
		}
		return(first.Filename < second.Filename);
	});

	return(records);
}
