/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "_timer.h"
#include "mono.h"
#include "utf8.h"

#include <cstddef>
#include <cstring>

TTimerClass<SystemTimerClass> TickCount;
MonoClass Mono;

int SystemTimerClass::operator () (void) const { return(0); }
SystemTimerClass::operator int(void) const { return(0); }
void __cdecl DebugString(char const *, ...) {}
void __cdecl DebugStringNoPrefix(char const *, ...) {}
std::size_t UTF8::Copy(char *destination, std::size_t capacity, char const *source)
{
	if (capacity == 0) return(0);
	std::strncpy(destination, source, capacity - 1);
	destination[capacity - 1] = '\0';
	return(std::strlen(destination));
}
MonoClass::MonoClass(void) {}
MonoClass::~MonoClass(void) {}
void MonoClass::Clear(void) {}
void MonoClass::Set_Cursor(int, int) {}
void __cdecl MonoClass::Printf(char const *, ...) {}
