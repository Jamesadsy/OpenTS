/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "userdatageneration.h"

#include <atomic>
#include <cctype>
#include <string>

namespace {
std::atomic<unsigned int> UserDataDirtyGeneration{0};
std::string UserDataRoot;
}

extern "C" void OpenTS_User_Data_Mark_Dirty(void)
{
	UserDataDirtyGeneration.fetch_add(1, std::memory_order_release);
}

extern "C" unsigned int OpenTS_User_Data_Dirty_Generation(void)
{
	return(UserDataDirtyGeneration.load(std::memory_order_acquire));
}

void OpenTS_User_Data_Set_Root(char const * root)
{
	UserDataRoot = root != nullptr ? root : "";
}

bool Is_User_Data_Path(char const * filename)
{
	if (filename == nullptr || UserDataRoot.empty()) return(false);
	std::string const path(filename);
	if (path.size() < UserDataRoot.size()) return(false);

	for (std::size_t i = 0; i < UserDataRoot.size(); ++i) {
		char left = path[i];
		char right = UserDataRoot[i];
		if (left == '/') left = '\\';
		if (right == '/') right = '\\';
		if (std::tolower(static_cast<unsigned char>(left))
			!= std::tolower(static_cast<unsigned char>(right))) return(false);
	}

	return(path.size() == UserDataRoot.size()
		|| UserDataRoot.back() == '/' || UserDataRoot.back() == '\\'
		|| path[UserDataRoot.size()] == '/' || path[UserDataRoot.size()] == '\\');
}
