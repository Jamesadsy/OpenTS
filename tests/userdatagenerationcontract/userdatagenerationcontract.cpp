/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "userdatageneration.h"

#include <cstdio>

int main(void)
{
	OpenTS_User_Data_Set_Root("/userdata/");
	bool const inside = Is_User_Data_Path("/userdata/Saved Games/Tiberian Sun/SAVE0001.SAV");
	bool const case_and_separator_safe = Is_User_Data_Path("\\USERDATA\\settings.ini");
	bool const sibling_prefix_rejected = !Is_User_Data_Path("/userdata-other/settings.ini");
	bool const root_itself = Is_User_Data_Path("/userdata/");
	unsigned int const initial = OpenTS_User_Data_Dirty_Generation();
	OpenTS_User_Data_Mark_Dirty();
	unsigned int const after_first = OpenTS_User_Data_Dirty_Generation();
	OpenTS_User_Data_Mark_Dirty();
	unsigned int const after_second = OpenTS_User_Data_Dirty_Generation();
	bool const pass = inside && case_and_separator_safe && sibling_prefix_rejected && root_itself
		&& after_first == initial + 1 && after_second == after_first + 1;
	std::printf("user-data path boundary and dirty generation are correct: %s\n", pass ? "ok" : "FAILED");
	return(pass ? 0 : 1);
}
