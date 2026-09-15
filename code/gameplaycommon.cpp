/*******************************************************************************
 *                                O P E N  T S
 *******************************************************************************
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright 2026 OpenTS contributors
 *
 * See LICENSE.md for applicable additional terms and warranty disclaimers.
 ******************************************************************************/

#include "always.h"

#include "gameplaycommon.h"

#include "data.h"
#include "globals.h"
#include "house.h"
#include "mapgen.h"
#include "session.h"
#include "special.h"

#include <cstddef>


/// <summary>
/// Computes a hash value for a string.
/// This routine supplies the bucket value for dictionaries that key by name.
/// </summary>
unsigned int Wstring_Hash(Wstring & string)
{
	unsigned int hash = string.length();

	for (unsigned int i = 0; i < string.length(); i++) {
		hash += *(string.get() + i);
		hash += i;
		hash = (hash << 8) ^ (hash >> 24);
	}
	return(hash);
}


/// <summary>
/// Counts the human teams still in the game.
/// A group of allied players counts as one team, leaving the supplied house out of the tally.
/// </summary>
int CountAliveTeams(HouseClass * house)
{
	int count = 0;
	for (int i = 0; i < Houses.Count(); i++) {
		HouseClass * house1 = Houses[i];
		if (house1 != NULL && !house1->IsDefeated && house1->IsHuman && house1 != house) {
			bool has_ally = false;
			for (int j = 0; j < Houses.Count(); j++) {
				if (j != i) {
					HouseClass * house2 = Houses[j];
					if (house2 != NULL && !house2->IsDefeated && house2->IsHuman) {
						if (house2 != house && house1->Is_Ally(Houses[j]) && house2->Is_Ally(house1)) {
							if (j > i && !has_ally) {
								count++;
							}
							has_ally = true;
						}
					}
				}
			}
			if (!has_ally) {
				count++;
			}
		}
	}
	return(count);
}


/// <summary>
/// Puts the session options the simulation reads into the shared special-rule state.
/// </summary>
void Commit_Session_Specials(void)
{
	Special.IsHarvesterImmune = Session.Options.HarvTruce;
	Special.IsDestroyBridges = Session.Options.BridgeDestruction;
	Special.IsScrapMetal = Session.Options.ScrapMetal;
	Special.IsTGrowth = true;
	Special.IsTSpread = true;
	Special.Apply_To_Game();
}


/// <summary>
/// Fetches a digest string for the current random-map seed.
/// The map description is deliberately excluded, so renaming a seed does not change its map.
/// </summary>
void CalcRandomMapDigest(char * digest, int bufsize)
{
	unsigned char * data;
	char description[sizeof(RandomMapGen.SeedData.MapDescription)];
	int size;
	unsigned char *bytes;
	unsigned int val;
	unsigned int hibit;
	unsigned int checksum = 0;

	memcpy(description, RandomMapGen.SeedData.MapDescription, sizeof(description));

	// Hash all of SeedData except UseTransitions.
	data = (unsigned char *)&RandomMapGen.SeedData.Biome;
	size = (sizeof(RandomMapGen.SeedData) - offsetof(MapSeedClass, Biome) - sizeof(RandomMapGen.SeedData.UseTransitions));
	memset(RandomMapGen.SeedData.MapDescription, '\0', sizeof(description));

	while (size > 0) {
		hibit = checksum >> 31;
		if (size >= 4) {
			val = (*(unsigned int *)data);
			checksum <<= 1;
			checksum += val;
			checksum += hibit;
			data += sizeof(unsigned int);
			size -= sizeof(unsigned int);
		} else {
			val = 0;
			bytes = data;
			while (size) {
				val <<= 8;
				val |= (*bytes++);
				size--;
			}
			checksum <<= 1;
			checksum += val;
			checksum += hibit;
		}
	}

	snprintf(digest, bufsize, "%08X", checksum);
	memcpy(RandomMapGen.SeedData.MapDescription, description, sizeof(description));
}
