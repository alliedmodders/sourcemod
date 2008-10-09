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
native NominateResult:NominateMap(const String:map[], bool:force, owner);

/**
 * Gets the current list of excluded maps.
 *
 * @param array		An ADT array handle to add the map strings to. Needs to be 
 * @noreturn
 */
native GetExcludeMapList(Handle:array);

/**
 * Checks if MapChooser will allow a vote
 *
 * @return			True if a vote can be held, or false if mapchooser is already holding a vote.
 */
native bool:CanMapChooserStartVote();

/**
 * Initiates a MapChooser map vote
 *
 * Note: If no input array is specified mapchooser will use its internal list. This includes
 * any nominations and excluded maps (as per mapchoosers convars).
 *
 * @param when			MapChange consant of when the resulting mapchange should occur.
 * @param inputarray	ADT array list of maps to add to the vote.
 */
native InitiateMapChooserVote(MapChange:when, Handle:inputarray=INVALID_HANDLE);

/**
 * Checks if MapChooser's end of map vote has completed.
 *
 * @return			True if complete, false otherwise.
 */
native bool:HasEndOfMapVoteFinished();

/**
 * Checks if MapChooser is set to run an end of map vote.
 *
 * @return			True if enabled, false otherwise.
 */
native bool:EndOfMapVoteEnabled();

/**
 * Called when mapchooser removes a nomination from its list.
 * Nominations cleared on map start will not trigger this forward
 */
forward OnNominationRemoved(const String:map[], owner);


public SharedPlugin:__pl_mapchooser = 
{
	name = "mapchooser",
	file = "mapchooser.smx",
#if defined REQUIRE_PLUGIN
	required = 1,
#else
	required = 0,
#endif
};

public __pl_mapchooser_SetNTVOptional()
{
	MarkNativeAsOptional("NominateMap");
	MarkNativeAsOptional("GetExcludeMapList");
	MarkNativeAsOptional("CanMapChooserStartVote");
	MarkNativeAsOptional("InitiateMapChooserVote");
	MarkNativeAsOptional("HasEndOfMapVoteFinished");
	MarkNativeAsOptional("EndOfMapVoteEnabled");
}

