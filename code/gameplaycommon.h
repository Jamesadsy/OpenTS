/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "wstring.h"

class HouseClass;

unsigned int Wstring_Hash(Wstring & string);
int CountAliveTeams(HouseClass * house);
void Commit_Session_Specials(void);

// Eight hexadecimal digits, a terminator, and slack.
constexpr int RANDOM_MAP_DIGEST_SIZE = 12;

void CalcRandomMapDigest(char * digest, int bufsize);
