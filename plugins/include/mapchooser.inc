/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2014 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */
#if defined _mapchooser_included_
  #endinput
#endif
#define _mapchooser_included_

enum NominateResult
{
	Nominate_Added,			/** The map was added to the nominate list */
	Nominate_Replaced,		/** A clients existing nomination was replaced */
	Nominate_AlreadyInVote,	/** Specified map was already in the vote */
	Nominate_InvalidMap,	/** Mapname specifed wasn't a valid map */
	Nominate_VoteFull,		/** This will only occur if force was set to false */
};

enum MapChange
{
	MapChange_Instant,		/** Change map as soon as the voting results have come in */
	MapChange_RoundEnd,		/** Change map at the end of the round */
	MapChange_MapEnd,		/** Change the sm_nextmap cvar */
};

/**
 * Attempt to add a map to the mapchooser map list.
 *
 * @param map		Map to add.
 * @param force		Should we force the map in even if it requires overwriting an existing nomination?
 * @param owner		Client index of the nominater. If the client disconnects the nomination will be removed. Use 0 for constant nominations
 * @return			Nominate Result of the outcome
 */
native NominateResult NominateMap(const char[] map, bool force, int owner);

/**
 * Attempt to remove a map from the mapchooser map list.
 *
 * @param map		Map to remove.
 * @return			True if the nomination was found and removed, or false if the nomination was not found.
 */
native bool RemoveNominationByMap(const char[] map);

/**
 * Attempt to remove a map from the mapchooser map list.
 *
 * @param owner		Client index of the nominater.
 * @return			True if the nomination was found and removed, or false if the nomination was not found.
 */
native bool RemoveNominationByOwner(int owner);

/**
 * Gets the current list of excluded maps.
 *
 * @param array		An ADT array handle to add the map strings to.
 * @noreturn
 */
native void GetExcludeMapList(ArrayList array);

/**
 * Gets the current list of nominated maps.
 *
 * @param maparray		An ADT array handle to add the map strings to.
 * @param ownerarray	An optional ADT array handle to add the nominator client indexes to.
 * @noreturn
 */
native void GetNominatedMapList(ArrayList maparray, ArrayList ownerarray = null);

/**
 * Checks if MapChooser will allow a vote
 *
 * @return			True if a vote can be held, or false if mapchooser is already holding a vote.
 */
native bool CanMapChooserStartVote();

/**
 * Initiates a MapChooser map vote
 *
 * Note: If no input array is specified mapchooser will use its internal list. This includes
 * any nominations and excluded maps (as per mapchoosers convars).
 *
 * @param when			MapChange consant of when the resulting mapchange should occur.
 * @param inputarray	ADT array list of maps to add to the vote.
 */
native void InitiateMapChooserVote(MapChange when, ArrayList inputarray=null);

/**
 * Checks if MapChooser's end of map vote has completed.
 *
 * @return			True if complete, false otherwise.
 */
native bool HasEndOfMapVoteFinished();

/**
 * Checks if MapChooser is set to run an end of map vote.
 *
 * @return			True if enabled, false otherwise.
 */
native bool EndOfMapVoteEnabled();

/**
 * Called when mapchooser removes a nomination from its list.
 * Nominations cleared on map start will not trigger this forward
 */
forward void OnNominationRemoved(const char[] map, int owner);

/**
 * Called when mapchooser starts a Map Vote.
 */
forward void OnMapVoteStarted();

/**
 * Given a map list from ReadMapList, attempt to resolve all fuzzy map names
 * This is used internally by MapChooser and its children.
 * 
 * @param mapList			A maplist produced by ReadMapList
 * @param resolvedMapList	An empty ArrayList that will be populated with the
 * 							resolved names of the fuzzy map names in mapList
 * 							If null, will be created.
 */
stock void ProduceResolvedMapList(ArrayList mapList, ArrayList resolvedMapList)
{
	if (mapList == null)
	{
		return;
	}
	
	if (resolvedMapList == null)
	{
		resolvedMapList = new ArrayList(ByteCountToCells(PLATFORM_MAX_PATH));
	}
	else
	{
		resolvedMapList.Clear();
	}
	
	int length = mapList.Length;
	for (int i = 0; i < length; i++)
	{
		char map[PLATFORM_MAX_PATH];
		char resolvedMap[PLATFORM_MAX_PATH];
		
		mapList.GetString(i, map, sizeof(map));
		
		if (ResolveFuzzyMapName(map, resolvedMap, sizeof(resolvedMap)))
		{
			resolvedMapList.PushString(resolvedMap);
		}
		else
		{
			resolvedMapList.PushString(map);
		}
	}
}

/**
 * This is used to get the Friendly name for a Workshop map.  Used for the display name in a map vote.
 * 
 * Note: non-Workshop maps will be ignored.
 * 
 * @param resolvedMap		A map name returned from ResolveFuzzyMapName
 * @param friendlyName		The map name with the extra workshop path bits removed
 * @param maxlength			The max length of friendlyName
 * 							(We recommend PLATFORM_MAX_PATH)
 * @param bIsMapResolved	True if map is a resolved Fuzzy Name, false if we need to resolve it first.
 * 							This is more or less a convenience method
 */
stock void GetFriendlyMapName(const char[] resolvedMap, char[] friendlyName, int maxlength, bool bIsMapResolved=true)
{
	int length = strlen(resolvedMap);
	char[] szTmp = new char[length];

	if (bIsMapResolved || !ResolveFuzzyMapName(resolvedMap, szTmp, length))
	{
		strcopy(szTmp, length, resolvedMap);
	}
	
	EngineVersion version = GetEngineVersion();
	switch (version)
	{
		case Engine_TF2:
		{
			int ugcPos;
			
			// In TF2, workshop maps show up as workshop/mapname.ugc123456789
			if (strncmp(szTmp, "workshop/", 9) == 0 && (ugcPos = StrContains(szTmp, ".ugc")) > -1)
			{
				// technically, this is (ugcPos + 1) - 9, but lets just cancel the 1.
				strcopy(friendlyName, maxlength < ugcPos - 8 ? maxlength : ugcPos - 8, szTmp[9]);
				return;
			}
		}
	}
	
	// Fallback to just copying the name back to itself
	strcopy(friendlyName, maxlength, szTmp);
}

public SharedPlugin __pl_mapchooser = 
{
	name = "mapchooser",
	file = "mapchooser.smx",
#if defined REQUIRE_PLUGIN
	required = 1,
#else
	required = 0,
#endif
};

public void __pl_mapchooser_SetNTVOptional()
{
	MarkNativeAsOptional("NominateMap");
	MarkNativeAsOptional("RemoveNominationByMap");
	MarkNativeAsOptional("RemoveNominationByOwner");
	MarkNativeAsOptional("GetExcludeMapList");
	MarkNativeAsOptional("GetNominatedMapList");
	MarkNativeAsOptional("CanMapChooserStartVote");
	MarkNativeAsOptional("InitiateMapChooserVote");
	MarkNativeAsOptional("HasEndOfMapVoteFinished");
	MarkNativeAsOptional("EndOfMapVoteEnabled");
}

