/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#pragma once

#include "autosave.h"

#include <array>
#include <string>
#include <vector>


class ManualSaveIdentityClass
{
	public:
		using ProductType = AutosaveClass::ProductType;

		void Record_Load(ProductType product, std::string const & filename, bool succeeded)
		{
			if (succeeded) {
				Current[Product_Index(product)] = filename;
			}
		}

		void Record_Save(ProductType product, std::string const & filename, bool succeeded)
		{
			if (succeeded) {
				Current[Product_Index(product)] = filename;
			}
		}

		std::string const & File_Name(ProductType product) const
		{
			return(Current[Product_Index(product)]);
		}

		void Clear(void)
		{
			for (std::string & filename : Current) {
				filename.clear();
			}
		}

		void Clear(ProductType product)
		{
			Current[Product_Index(product)].clear();
		}

	private:
		static std::size_t Product_Index(ProductType product)
		{
			return(product == ProductType::Firestorm ? 1 : 0);
		}

		std::array<std::string, 2> Current{};
};


template <typename EntryVector>
int Find_Save_Identity_Row(std::string const & filename, EntryVector const & entries)
{
	if (filename.empty()) {
		return(-1);
	}

	for (std::size_t index = 0; index < entries.size(); index++) {
		if (entries[index].Valid && entries[index].Filename == filename) {
			return(static_cast<int>(index));
		}
	}
	return(-1);
}


template <typename EntryVector>
int Initial_Save_Identity_Row(std::string const & filename, EntryVector const & entries)
{
	int const current = Find_Save_Identity_Row(filename, entries);
	return(current >= 0 ? current : entries.empty() ? -1 : 0);
}

// A manual save targets the file identity captured by the selected list row. The
// description is intentionally absent: two rows may have the same description.
inline std::string Save_Target_Filename(bool selected_is_save,
	std::string const & selected_filename, std::string const & new_slot_filename)
{
	return(selected_is_save ? selected_filename : new_slot_filename);
}
