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
#include <string>


inline int Save_Browser_Next_Row(int selected, std::size_t row_count, int direction)
{
	if (row_count == 0 || direction == 0) {
		return(row_count == 0 ? -1 : selected);
	}

	if (selected < 0 || selected >= static_cast<int>(row_count)) {
		return(direction < 0 ? static_cast<int>(row_count) - 1 : 0);
	}
	if (direction < 0) {
		return(selected == 0 ? static_cast<int>(row_count) - 1 : selected - 1);
	}
	return((selected + 1) % static_cast<int>(row_count));
}


template <typename EntryVector>
std::string Save_Browser_Description(int selected, EntryVector const & entries,
	std::string const & new_save_description)
{
	if (selected >= 0 && selected < static_cast<int>(entries.size()) && entries[selected].Valid) {
		return(entries[selected].Description);
	}
	return(new_save_description);
}


template <typename ScrollFunction>
void Save_Browser_Scroll_Selected_Row(int selected, std::size_t row_count, ScrollFunction scroll)
{
	if (selected >= 0 && selected < static_cast<int>(row_count)) {
		scroll(selected);
	}
}
