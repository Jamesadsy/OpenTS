/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "landiagnostics.h"

#include <array>
#include <cstdio>
#include <cstdlib>
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

	char const * const trace_path = std::getenv("OPENTS_LAN_TRACE_PATH");
	if (trace_path != nullptr && trace_path[0] != '\0') {
		FILE * const trace = std::fopen(trace_path, "wb");
		if (trace != nullptr) {
			std::fputs("sequence\tlayer\tcode\tdetail0\tdetail1\n", trace);
			std::uint32_t const first = NextSequence - static_cast<std::uint32_t>(EventCount);
			for (std::size_t index = 0; index < EventCount; ++index) {
				Event const & retained = Events[(first + static_cast<std::uint32_t>(index) - 1) % CAPACITY];
				std::fprintf(trace, "%u\t%s\t%s\t%d\t%d\n", retained.Sequence,
					Layer_Name(retained.EventLayer), Code_Name(retained.EventCode),
					retained.Detail0, retained.Detail1);
			}
			std::fclose(trace);
		}
	}
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

char const * Code_Name(Code code) noexcept
{
	switch (code) {
		case Code::STARTED: return("STARTED");
		case Code::PERMISSION_NOT_REQUESTED: return("PERMISSION_NOT_REQUESTED");
		case Code::READY: return("READY");
		case Code::ALLOWED_NO_HOST: return("ALLOWED_NO_HOST");
		case Code::DISCOVERY_ERROR: return("DISCOVERY_ERROR");
		case Code::UDP_TRANSPORT_ERROR: return("UDP_TRANSPORT_ERROR");
		case Code::FAILED: return("FAILED");
		case Code::DENIED: return("DENIED");
		case Code::NO_HOST: return("NO_HOST");
		case Code::ENDPOINT_FOUND: return("ENDPOINT_FOUND");
		case Code::ENDPOINT_REJECTED: return("ENDPOINT_REJECTED");
		case Code::QUERY_GAME: return("QUERY_GAME");
		case Code::ANSWER_GAME: return("ANSWER_GAME");
		case Code::JOIN_REQUEST: return("JOIN_REQUEST");
		case Code::JOIN_CONFIRMED: return("JOIN_CONFIRMED");
		case Code::JOIN_REJECTED: return("JOIN_REJECTED");
		case Code::LOBBY_OPEN: return("LOBBY_OPEN");
		case Code::LOBBY_CHANGED: return("LOBBY_CHANGED");
		case Code::CHAT_SENT: return("CHAT_SENT");
		case Code::SCENARIO_MATCH: return("SCENARIO_MATCH");
		case Code::SCENARIO_TRANSFER: return("SCENARIO_TRANSFER");
		case Code::SCENARIO_REJECTED: return("SCENARIO_REJECTED");
		case Code::GAME_READY: return("GAME_READY");
		case Code::GAME_GO: return("GAME_GO");
		case Code::FRAME_SYNC_WAIT: return("FRAME_SYNC_WAIT");
		case Code::COMMAND_COUNT_WAIT: return("COMMAND_COUNT_WAIT");
		case Code::CHECKSUM_MISMATCH: return("CHECKSUM_MISMATCH");
		default: return("UNKNOWN");
	}
}

} // namespace LANDiagnostics
