/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#pragma once

#include "lanbootstrap.h"

#include <cstdint>

namespace IOSLanDiscovery
{
	enum class State : std::uint8_t
	{
		PERMISSION_NOT_REQUESTED,
		REQUESTED,
		SEARCHING,
		ALLOWED_NO_HOST,
		DENIED,
		DISCOVERY_ERROR,
		ENDPOINT_FOUND,
		UDP_ERROR
	};

	inline constexpr char SERVICE_TYPE[] = "_secondsun._udp";

	void Start_Browsing();
	void Stop_Browsing();
	void Start_Advertising(std::uint16_t udp_port);
	void Stop_Advertising();
	bool Pop_Endpoint(LANBootstrap::Endpoint & endpoint);
	State Current_State();
	char const * Status_Text();
	void Record_UDP_Error(int error_code);
}
