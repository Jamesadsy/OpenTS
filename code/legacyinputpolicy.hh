/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once


template <typename Service, typename DiscardActions, typename MessageDrain>
void MSEngine_Service_Legacy_Input(Service && service, DiscardActions && discard_actions,
	MessageDrain && message_drain)
{
	service();
	discard_actions();
	message_drain();
}
