/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#pragma once

// A successful mutation under /userdata advances this process-local generation. The
// browser owner samples it after each cooperative native Step and persists only when it
// changes. Reads, failed writes, and the read-only /data tree do not advance it.
extern "C" void OpenTS_User_Data_Mark_Dirty(void);
extern "C" unsigned int OpenTS_User_Data_Dirty_Generation(void);

void OpenTS_User_Data_Set_Root(char const * root);
bool Is_User_Data_Path(char const * filename);
