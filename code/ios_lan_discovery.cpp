/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 ******************************************************************************/

#include "ios_lan_discovery.h"

#if !defined(OPENTS_IOS)
namespace IOSLanDiscovery
{
void Start_Browsing() {}
void Stop_Browsing() {}
void Start_Advertising(std::uint16_t) {}
void Stop_Advertising() {}
bool Pop_Endpoint(LANBootstrap::Endpoint &) { return(false); }
State Current_State() { return(State::PERMISSION_NOT_REQUESTED); }
char const * Status_Text() { return("Bonjour discovery is not active on this platform."); }
void Record_UDP_Error(int) {}
}
#endif
