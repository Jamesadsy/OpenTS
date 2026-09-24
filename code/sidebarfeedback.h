/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

// Sidebar paint state is a projection of the native display modes. It does not
// own or retain a second repair/sell mode.
struct RepairSellSidebarFeedbackType
{
	bool RepairOn;
	bool SellOn;
};

constexpr RepairSellSidebarFeedbackType RepairSell_Sidebar_Feedback_From_Native_Flags(
	bool is_repair_mode, bool is_sell_mode)
{
	return { is_repair_mode, is_sell_mode };
}
