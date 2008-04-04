#if defined _tf2_included
 #endinput
#endif
#define _tf2_included

enum TFClassType
{
	TFClass_Unknown = 0,
	TFClass_Scout,
	TFClass_Sniper,
	TFClass_Soldier,
	TFClass_DemoMan,
	TFClass_Medic,
	TFClass_Heavy,
	TFClass_Pyro,
	TFClass_Spy,
	TFClass_Engineer
}

enum TFTeam
{
	TFTeam_Unassigned = 0,
	TFTeam_Spectator = 1,
	TFTeam_Red = 2,
	TFTeam_Blue = 3	
}

/**
 * Set's a Clients invulnrability status (ubercharge effect)
 *
 * @param client		Player's index.
 * @param enabled		Enable/Disable invulnrability.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_SetPlayerInvuln(client, bool:enabled);


/**
 * Sets a client on fire for 10 seconds.
 *
 * @param client		Player's index.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_IgnitePlayer(client, target);

/**
 * Respawns a client
 *
 * @param client		Player's index.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_RespawnPlayer(client);

/**
 * Disguises a client to the given model and team. Only has an effect on spies.
 *
 * Note: This only starts the disguise process and a delay occurs before the spy is fully disguised
 *
 * @param client		Player's index.
 * @param team			Team to disguise the player as (only TFTeam_Red and TFTeam_Blue have an effect)
 * @param class			TFClassType class to disguise the player as
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_DisguisePlayer(client, TFTeam:team, TFClassType:class);

/**
 * Removes the current disguise from a client. Only has an effect on spies.
 *
 * @param client		Player's index.
 * @noreturn
 * @error				Invalid client index, client not in game, or no mod support.
 */
native TF2_RemovePlayerDisguise(client);

/**
 * Retrieves the entity index of the CPlayerResource entity
 *
 * @return				The current resource entity index.
 */
native TF2_GetResourceEntity();

/**
 * Finds the TFClassType for a given class name.
 *
 * @param classname		A classname string such as "sniper" or "demoman"
 * @return				A TFClassType constant.
 */
native TFClassType:TF2_GetClass(const String:classname[]);

/**
 * Do not edit below this line!
 */
public Extension:__ext_tf2 = 
{
	name = "TF2 Tools",
	file = "game.tf2.ext",
	autoload = 1,
#if defined REQUIRE_EXTENSIONS
	required = 1,
#else
	required = 0,
#endif
};
