/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include <string>

// A manual save targets the file identity captured by the selected list row. The
// description is intentionally absent: two rows may have the same description.
inline std::string Save_Target_Filename(bool selected_is_save,
	std::string const & selected_filename, std::string const & new_slot_filename)
{
	return(selected_is_save ? selected_filename : new_slot_filename);
}
