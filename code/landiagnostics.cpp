/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "landiagnostics.h"

#include <array>
#include <mutex>

namespace LANDiagnostics
{
namespace
{
std::mutex EventMutex;
std::array<Event, CAPACITY> Events{};
std::uint32_t NextSequence = 1;
std::size_t EventCount = 0;
}

void Record(Layer layer, Code code, std::int32_t detail0, std::int32_t detail1) noexcept
{
	std::lock_guard<std::mutex> lock(EventMutex);
	Event const event{NextSequence++, layer, code, detail0, detail1};
	Events[(event.Sequence - 1) % CAPACITY] = event;
	if (EventCount < CAPACITY) ++EventCount;
}

std::size_t Copy_Last(Event * destination, std::size_t capacity) noexcept
{
	if (destination == nullptr || capacity == 0) return(0);
	std::lock_guard<std::mutex> lock(EventMutex);
	std::size_t const count = EventCount < capacity ? EventCount : capacity;
	std::uint32_t const first = NextSequence - static_cast<std::uint32_t>(count);
	for (std::size_t index = 0; index < count; ++index) {
		destination[index] = Events[(first + static_cast<std::uint32_t>(index) - 1) % CAPACITY];
	}
	return(count);
}

void Reset_For_Test() noexcept
{
	std::lock_guard<std::mutex> lock(EventMutex);
	Events = {};
	NextSequence = 1;
	EventCount = 0;
}

char const * Layer_Name(Layer layer) noexcept
{
	switch (layer) {
		case Layer::SOCKET_BIND: return("SOCKET_BIND");
		case Layer::DISCOVERY: return("DISCOVERY");
		case Layer::ADDRESS_SELECTION: return("ADDRESS_SELECTION");
		case Layer::JOIN_HANDSHAKE: return("JOIN_HANDSHAKE");
		case Layer::LOBBY_STATE: return("LOBBY_STATE");
		case Layer::MAP_SCENARIO: return("MAP_SCENARIO");
		case Layer::GAME_START_TICK: return("GAME_START_TICK");
		case Layer::STALL: return("STALL");
		case Layer::DESYNC: return("DESYNC");
		default: return("UNKNOWN");
	}
}

} // namespace LANDiagnostics
