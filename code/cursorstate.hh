/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "action.hh"
#include "mouse.hh"


enum class RepairSellModeCursor {
	Normal,
	Repair,
	Sell
};


constexpr MouseType RepairSell_Mode_Cursor_Seed(RepairSellModeCursor mode)
{
	switch (mode) {
		case RepairSellModeCursor::Repair:
			return(MOUSE_REPAIR);

		case RepairSellModeCursor::Sell:
			return(MOUSE_SELL_BACK);

		default:
			return(MOUSE_NORMAL);
	}
}


constexpr RepairSellModeCursor Next_RepairSell_Mode_Cursor(RepairSellModeCursor mode)
{
	switch (mode) {
		case RepairSellModeCursor::Repair:
			return(RepairSellModeCursor::Sell);

		case RepairSellModeCursor::Sell:
			return(RepairSellModeCursor::Normal);

		default:
			return(RepairSellModeCursor::Repair);
	}
}


constexpr MouseType Action_Cursor_Shape(ActionType action, bool shadow, bool move_to_shroud, bool attack_in_range)
{
	if (shadow) {
		switch (action) {
			case ACTION_TOTE:
				return(MOUSE_NO_TOTE);

			case ACTION_NO_ENTER:
			case ACTION_NO_ENTER_TUNNEL:
				return(MOUSE_NO_ENTER);

			case ACTION_DAMAGE:
			case ACTION_GREPAIR:
				return(MOUSE_NORMAL);

			case ACTION_NO_DEPLOY:
				return(MOUSE_NO_DEPLOY);

			case ACTION_GUARD_AREA:
				return(MOUSE_AREA_GUARD);

			case ACTION_CHEM_BOMB:
				return(MOUSE_CHEMBOMB);

			case ACTION_NO_SELL:
			case ACTION_SELL:
			case ACTION_SELL_UNIT:
				return(MOUSE_NO_SELL_BACK);

			case ACTION_NO_GREPAIR:
			case ACTION_NO_REPAIR:
			case ACTION_REPAIR:
				return(MOUSE_NO_REPAIR);

			case ACTION_NUKE_BOMB:
				return(MOUSE_NUCLEAR_BOMB);

			case ACTION_TOGGLE_POWER:
			case ACTION_NO_TOGGLE_POWER:
				return(MOUSE_NO_TOGGLE_POWER);

			case ACTION_EMPULSE:
				return(MOUSE_EM_PULSE);

			case ACTION_ION_CANNON:
			case ACTION_DROP_POD:
				return(MOUSE_AIR_STRIKE);

			case ACTION_EMPULSE_RANGE:
				return(MOUSE_EM_PULSE_RANGE);

			case ACTION_HEAL:
				return(MOUSE_HEAL);

			case ACTION_NOMOVE:
				return(move_to_shroud ? MOUSE_CAN_MOVE : MOUSE_NO_MOVE);

			case ACTION_MOVE:
			case ACTION_ATTACK:
				return(MOUSE_CAN_MOVE);

			case ACTION_PLACE_WAYPOINT:
				return(MOUSE_PLACE_WAYPOINT);

			case ACTION_NO_PLACE_WAYPOINT:
				return(MOUSE_NO_PLACE_WAYPOINT);

			case ACTION_ENTER_WAYPOINT_MODE:
				return(MOUSE_ENTER_WAYPOINT_MODE);

			case ACTION_SELECT_WAYPOINT:
				return(MOUSE_SELECT_WAYPOINT);

			case ACTION_LOOP_WAYPOINT_PATH:
				return(MOUSE_LOOP_WAYPOINT_PATH);

			case ACTION_ATTACK_WAYPOINT:
				return(MOUSE_ATTACK_WAYPOINT);

			case ACTION_PATROL_WAYPOINT:
				return(MOUSE_PATROL_WAYPOINT);

			case ACTION_FOLLOW_WAYPOINT:
				return(MOUSE_FOLLOW_WAYPOINT);

			case ACTION_ENTER_WAYPOINT:
				return(MOUSE_ENTER_WAYPOINT);

			default:
				return(MOUSE_NORMAL);
		}
	}

	switch (action) {
		case ACTION_TOTE:
			return(MOUSE_TOTE);

		case ACTION_NO_ENTER:
			return(MOUSE_NO_ENTER);

		case ACTION_GREPAIR:
			return(MOUSE_GREPAIR);

		case ACTION_TOGGLE_SELECT:
		case ACTION_SELECT:
			return(MOUSE_CAN_SELECT);

		case ACTION_NO_DEPLOY:
			return(MOUSE_NO_DEPLOY);

		case ACTION_GUARD_AREA:
			return(MOUSE_AREA_GUARD);

		case ACTION_CHEM_BOMB:
			return(MOUSE_CHEMBOMB);

		case ACTION_MOVE:
		case ACTION_RALLY_TO_POINT:
			return(MOUSE_CAN_MOVE);

		case ACTION_ATTACK:
			return(attack_in_range ? MOUSE_STAY_ATTACK : MOUSE_CAN_ATTACK);

		case ACTION_HARVEST:
			return(MOUSE_CAN_ATTACK);

		case ACTION_SABOTAGE:
			return(MOUSE_DEMOLITIONS);

		case ACTION_ENTER:
		case ACTION_CAPTURE:
		case ACTION_ENTER_TUNNEL:
			return(MOUSE_ENTER);

		case ACTION_NOMOVE:
			return(MOUSE_NO_MOVE);

		case ACTION_NO_SELL:
			return(MOUSE_NO_SELL_BACK);

		case ACTION_NO_REPAIR:
		case ACTION_NO_GREPAIR:
			return(MOUSE_NO_REPAIR);

		case ACTION_SELF:
			return(MOUSE_DEPLOY);

		case ACTION_REPAIR:
			return(MOUSE_REPAIR);

		case ACTION_SELL_UNIT:
			return(MOUSE_SELL_UNIT);

		case ACTION_NO_TOGGLE_POWER:
			return(MOUSE_NO_TOGGLE_POWER);

		case ACTION_TOGGLE_POWER:
			return(MOUSE_TOGGLE_POWER);

		case ACTION_SELL:
			return(MOUSE_SELL_BACK);

		case ACTION_NUKE_BOMB:
			return(MOUSE_NUCLEAR_BOMB);

		case ACTION_EMPULSE:
			return(MOUSE_EM_PULSE);

		case ACTION_EMPULSE_RANGE:
			return(MOUSE_EM_PULSE_RANGE);

		case ACTION_ION_CANNON:
		case ACTION_DROP_POD:
			return(MOUSE_AIR_STRIKE);

		case ACTION_HEAL:
			return(MOUSE_HEAL);

		case ACTION_PLACE_WAYPOINT:
			return(MOUSE_PLACE_WAYPOINT);

		case ACTION_NO_PLACE_WAYPOINT:
			return(MOUSE_NO_PLACE_WAYPOINT);

		case ACTION_ENTER_WAYPOINT_MODE:
			return(MOUSE_ENTER_WAYPOINT_MODE);

		case ACTION_SELECT_WAYPOINT:
			return(MOUSE_SELECT_WAYPOINT);

		case ACTION_LOOP_WAYPOINT_PATH:
			return(MOUSE_LOOP_WAYPOINT_PATH);

		case ACTION_ATTACK_WAYPOINT:
			return(MOUSE_ATTACK_WAYPOINT);

		case ACTION_PATROL_WAYPOINT:
			return(MOUSE_PATROL_WAYPOINT);

		case ACTION_FOLLOW_WAYPOINT:
			return(MOUSE_FOLLOW_WAYPOINT);

		case ACTION_ENTER_WAYPOINT:
			return(MOUSE_ENTER_WAYPOINT);

		default:
			return(MOUSE_NORMAL);
	}
}
