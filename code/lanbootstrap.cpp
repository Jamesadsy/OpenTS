/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "lanbootstrap.h"

#include "netsocket.h"

#include <array>
#include <cstring>

namespace LANBootstrap
{
bool Parse_Private_IPv4(std::string_view text, std::uint16_t port,
	Endpoint & endpoint, ParseError * error) noexcept
{
	if (error != nullptr) *error = ParseError::NONE;
	auto reject = [error](ParseError reason) {
		if (error != nullptr) *error = reason;
		return(false);
	};
	if (text.empty()) return(reject(ParseError::EMPTY));
	if (port == 0) return(reject(ParseError::BAD_PORT));

	std::array<std::uint8_t, 4> octets{};
	std::size_t position = 0;
	for (std::size_t part = 0; part < octets.size(); ++part) {
		std::size_t const start = position;
		unsigned value = 0;
		while (position < text.size() && text[position] >= '0' && text[position] <= '9') {
			if (position - start == 3) return(reject(ParseError::SYNTAX));
			value = value * 10U + static_cast<unsigned>(text[position] - '0');
			if (value > 255U) return(reject(ParseError::OCTET_RANGE));
			++position;
		}
		std::size_t const digits = position - start;
		if (digits == 0 || (digits > 1 && text[start] == '0')) {
			return(reject(ParseError::SYNTAX));
		}
		octets[part] = static_cast<std::uint8_t>(value);
		if (part + 1 < octets.size()) {
			if (position >= text.size() || text[position] != '.') return(reject(ParseError::SYNTAX));
			++position;
		}
	}
	if (position != text.size()) return(reject(ParseError::SYNTAX));

	std::uint32_t address = 0;
	std::memcpy(&address, octets.data(), octets.size());
	if (!Is_Private_IPv4(address)) return(reject(ParseError::NOT_PRIVATE));

	endpoint = Endpoint{address, port};
	return(true);
}

bool Is_Private_IPv4(std::uint32_t address_network_order) noexcept
{
	std::array<std::uint8_t, 4> octets{};
	std::memcpy(octets.data(), &address_network_order, octets.size());
	return octets[0] == 10
		|| (octets[0] == 172 && octets[1] >= 16 && octets[1] <= 31)
		|| (octets[0] == 192 && octets[1] == 168);
}

int Address_Class(std::uint32_t address_network_order) noexcept
{
	std::array<std::uint8_t, 4> octets{};
	std::memcpy(octets.data(), &address_network_order, octets.size());
	if (octets[0] == 10) return(10);
	if (octets[0] == 172 && octets[1] >= 16 && octets[1] <= 31) return(172);
	if (octets[0] == 192 && octets[1] == 168) return(192);
	return(0);
}

IPXAddressClass To_IPX_Address(Endpoint const & endpoint) noexcept
{
	return(IPXAddressClass(endpoint.Address, Socket_Network_Port(endpoint.Port)));
}

bool Same_Endpoint(Endpoint const & left, Endpoint const & right) noexcept
{
	return(left.Address == right.Address && left.Port == right.Port);
}
}
