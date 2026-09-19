/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <SDL3/SDL.h>

// Private deterministic hooks used only by the owner-data-free G3 touch contract. They
// override host classification, time and window measurement without changing production
// gesture thresholds or event ownership.
void Win32_Touch_Test_Reset(void);
void Win32_Touch_Test_Set_Window_Size(float width, float height);
void Win32_Touch_Test_Set_Direct_Device(bool direct);
void Win32_Touch_Test_Set_Now(Uint64 now);
