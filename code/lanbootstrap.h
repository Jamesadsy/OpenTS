/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#pragma once

#include "ipxaddr.h"

#include <cstdint>
#include <string_view>

namespace LANBootstrap
{
	// IPv4 is stored in the same network byte order used by IPXAddressClass.
	struct Endpoint
	{
		std::uint32_t Address = 0;
		std::uint16_t Port = 0; // host byte order
	};

	enum class ParseError : std::uint8_t
	{
		NONE,
		EMPTY,
		SYNTAX,
		OCTET_RANGE,
		NOT_PRIVATE,
		BAD_PORT
	};

	bool Parse_Private_IPv4(std::string_view text, std::uint16_t port,
		Endpoint & endpoint, ParseError * error = nullptr) noexcept;
	bool Is_Private_IPv4(std::uint32_t address_network_order) noexcept;
	int Address_Class(std::uint32_t address_network_order) noexcept;
	IPXAddressClass To_IPX_Address(Endpoint const & endpoint) noexcept;
	bool Same_Endpoint(Endpoint const & left, Endpoint const & right) noexcept;

	// This is the single injection seam used by both the manual field and Bonjour
	// resolver. Keeping it templated lets its exact IPXManager call be exercised
	// by an asset-free contract fixture.
	template<class Manager>
	void Inject_Direct_Peer(Manager & manager, Endpoint const & endpoint)
	{
		manager.Add_Peer(To_IPX_Address(endpoint));
	}
}
