/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

namespace LANDiagnostics
{

// These are the stable layers used to attribute a later physical LAN failure.
enum class Layer : std::uint8_t
{
	SOCKET_BIND,
	DISCOVERY,
	ADDRESS_SELECTION,
	JOIN_HANDSHAKE,
	LOBBY_STATE,
	MAP_SCENARIO,
	GAME_START_TICK,
	STALL,
	DESYNC,
	COUNT
};

enum class Code : std::uint8_t
{
	STARTED = 1,
	PERMISSION_NOT_REQUESTED,
	READY,
	ALLOWED_NO_HOST,
	DISCOVERY_ERROR,
	UDP_TRANSPORT_ERROR,
	FAILED,
	DENIED,
	NO_HOST,
	ENDPOINT_FOUND,
	ENDPOINT_REJECTED,
	QUERY_GAME,
	ANSWER_GAME,
	JOIN_REQUEST,
	JOIN_CONFIRMED,
	JOIN_REJECTED,
	LOBBY_OPEN,
	LOBBY_CHANGED,
	CHAT_SENT,
	SCENARIO_MATCH,
	SCENARIO_TRANSFER,
	SCENARIO_REJECTED,
	GAME_READY,
	GAME_GO,
	FRAME_SYNC_WAIT,
	COMMAND_COUNT_WAIT,
	CHECKSUM_MISMATCH
};

// Numeric detail fields deliberately contain bounded values only. Do not put player names,
// full addresses, device identifiers, scenario paths, or secrets in this record.
struct Event
{
	std::uint32_t Sequence = 0;
	Layer EventLayer = Layer::SOCKET_BIND;
	Code EventCode = Code::STARTED;
	std::int32_t Detail0 = 0;
	std::int32_t Detail1 = 0;
};

inline constexpr std::size_t CAPACITY = 64;

void Record(Layer layer, Code code, std::int32_t detail0 = 0, std::int32_t detail1 = 0) noexcept;
std::size_t Copy_Last(Event * destination, std::size_t capacity) noexcept;
void Reset_For_Test() noexcept;
char const * Layer_Name(Layer layer) noexcept;

} // namespace LANDiagnostics
