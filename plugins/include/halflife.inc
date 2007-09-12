/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
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

#if defined _halflife_included
 #endinput
#endif
#define _halflife_included

#define MOTDPANEL_TYPE_TEXT		0	/**< Treat msg as plain text */
#define MOTDPANEL_TYPE_INDEX		1	/**< Msg is auto determined by the engine */
#define MOTDPANEL_TYPE_URL		2	/**< Treat msg as an URL link */
#define MOTDPANEL_TYPE_FILE		3	/**< Treat msg as a filename to be openned */

enum DialogType
{
	DialogType_Msg = 0,		/**< just an on screen message */
	DialogType_Menu, 		/**< an options menu */
	DialogType_Text, 		/**< a richtext dialog */
	DialogType_Entry, 		/**< an entry box */
	DialogType_AskConnect		/**< ask the client to connect to a specified IP */
};

/**
 * Logs a generic message to the HL2 logs.
 *
 * @param format		String format.
 * @param ...			Format arguments.
 * @noreturn
 */
native LogToGame(const String:format[], any:...);

/**
 * Sets the seed value for the global Half-Life 2 Random Stream.
 *
 * @param seed			Seed value.
 * @noreturn	
 */
native SetRandomSeed(seed);

/**
 * Returns a random floating point number from the Half-Life 2 Random Stream.
 *
 * @param fMin			Minimum random bound.
 * @param fMax			Maximum random bound.
 * @return				A random number between (inclusive) fMin and fMax.
 */
native Float:GetRandomFloat(Float:fMin=0.0, Float:fMax=1.0);

/**
 * Returns a random number from the Half-Life 2 Random Stream.
 *
 * @param nmin			Minimum random bound.
 * @param nmax			Maximum random bound.
 * @return				A random number between (inclusive) nmin and nmax.
 */
native GetRandomInt(nmin, nmax);

/**
 * Returns whether a map is valid or not.
 * 
 * @param 				Map name, excluding .bsp extension.
 * @return				True if valid, false otherwise.
 */
native bool:IsMapValid(const String:map[]);

/**
 * Returns whether the server is dedicated.
 *
 * @return				True if dedicated, false otherwise.
 */
native bool:IsDedicatedServer();

/**
 * Returns a high-precision time value for profiling the engine.
 *
 * @return				A floating point time value.
 */
native Float:GetEngineTime();

/** 
 * Returns the game time based on the game tick.
 *
 * @return				Game tick time.
 */
native Float:GetGameTime();

/**
 * Returns the game description from the mod.
 *
 * @param buffer		Buffer to store the description.
 * @param maxlength		Maximum size of the buffer.
 * @param original		If true, retrieves the original game description,
 *						ignoring any potential hooks from plugins.
 * @return				Number of bytes written to the buffer (UTF-8 safe).
 */
native GetGameDescription(String:buffer[], maxlength, bool:original=false);

/**
 * Returns the name of the game's directory.
 *
 * @param buffer		Buffer to store the directory name.
 * @param maxlength		Maximum size of the buffer.
 *
 * return				Number of bytes written to the buffer (UTF-8 safe).
 */
native GetGameFolderName(String:buffer[], maxlength);

/**
 * Returns the current map name.
 *
 * @param buffer		Buffer to store map name.
 * @param maxlength		Maximum length of buffer.
 * @return				Number of bytes written (UTF-8 safe).
 */
native GetCurrentMap(String:buffer[], maxlength);

/**
 * Precaches a given model.
 *
 * @param model			Name of the model to precache.
 * @param preload		If preload is true the file will be precached before level startup.
 * @return				Returns the model index, 0 for error.
 */
native PrecacheModel(const String:model[], bool:preload=false);

/**
 * Precaches a given sentence file.
 *
 * @param file			Name of the sentence file to precache.
 * @param preload		If preload is true the file will be precached before level startup.
 * @return				Returns a sentence file index.
 */
native PrecacheSentenceFile(const String:file[], bool:preload=false);

/**
 * Precaches a given decal.
 *
 * @param decal			Name of the decal to precache.
 * @param preload		If preload is true the file will be precached before level startup.
 * @return				Returns a decal index.
 */
native PrecacheDecal(const String:decal[], bool:preload=false);

/**
 * Precaches a given generic file.
 *
 * @param generic		Name of the generic file to precache.
 * @param preload		If preload is true the file will be precached before level startup.
 * @return				Returns a generic file index.
 */
native PrecacheGeneric(const String:generic[], bool:preload=false);

/**
 * Returns if a given model is precached.
 *
 * @param model			Name of the model to check.
 * @return				True if precached, false otherwise.
 */
native bool:IsModelPrecached(const String:model[]);

/**
 * Returns if a given decal is precached.
 *
 * @param decal			Name of the decal to check.
 * @return				True if precached, false otherwise.
 */
native bool:IsDecalPrecached(const String:decal[]);

/**
 * Returns if a given generic file is precached.
 *
 * @param decal			Name of the generic file to check.
 * @return				True if precached, false otherwise.
 */
native bool:IsGenericPrecached(const String:generic[]);

/**
 * Precaches a given sound.
 *
 * @param sound			Name of the sound to precache.
 * @param preload		If preload is true the file will be precached before level startup.
 * @return				True if successfully precached, false otherwise.
 */
native bool:PrecacheSound(const String:sound[], bool:preload=false);

/**
 * Returns if a given sound is precached.
 *
 * @param sound			Name of the sound to check.
 * @return				True if precached, false otherwise.
 */
native bool:IsSoundPrecached(const String:sound[]);

/**
 * Creates different types of ingame messages.
 *
 * @param client		Index of the client.
 * @param kv			KeyValues handle to set the menu keys and options. (Check iserverplugin.h for more information).
 * @param type			Message type to display ingame.
 * @noreturn
 * @error				Invalid client index, or client not connected.
 */
native CreateDialog(client, Handle:kv, DialogType:type);

/**
 * Prints a message to a specific client in the chat area.
 *
 * @param client		Client index.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 * @error				If the client is not connected an error will be thrown.
 */
native PrintToChat(client, const String:format[], any:...);

/**
 * Prints a message to all clients in the chat area.
 *
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 */
stock PrintToChatAll(const String:format[], any:...)
{
	new maxClients = GetMaxClients();
	decl String:buffer[192];
	
	for (new i = 1; i <= maxClients; i++)
	{
		if (IsClientInGame(i))
		{
			SetGlobalTransTarget(i);
			VFormat(buffer, sizeof(buffer), format, 2);
			PrintToChat(i, "%s", buffer);
		}
	}
}

/**
 * Prints a message to a specific client in the center of the screen.
 *
 * @param client		Client index.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 * @error				If the client is not connected an error will be thrown.
 */
native PrintCenterText(client, const String:format[], any:...);

/**
 * Prints a message to all clients in the center of the screen.
 *
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 */
stock PrintCenterTextAll(const String:format[], any:...)
{
	new maxClients = GetMaxClients();
	decl String:buffer[192];

	for (new i = 1; i <= maxClients; i++)
	{
		if (IsClientInGame(i))
		{
			SetGlobalTransTarget(i);
			VFormat(buffer, sizeof(buffer), format, 2);
			PrintCenterText(i, "%s", buffer);
		}
	}
}

/**
 * Prints a message to a specific client with a hint box.
 *
 * @param client		Client index.
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 * @error				If the client is not connected an error will be thrown.
 */
native PrintHintText(client, const String:format[], any:...);

/**
 * Prints a message to all clients with a hint box.
 *
 * @param format		Formatting rules.
 * @param ...			Variable number of format parameters.
 * @noreturn
 */
stock PrintHintTextToAll(const String:format[], any:...)
{
	new maxClients = GetMaxClients();
	decl String:buffer[192];
	
	for (new i = 1; i <= maxClients; i++)
	{
		if (IsClientInGame(i))
		{
			SetGlobalTransTarget(i);
			VFormat(buffer, sizeof(buffer), format, 2);
			PrintHintText(i, "%s", buffer);
		}
	}
}

/**
 * Shows a VGUI panel to a specific client.
 *
 * @param client		Client index.
 * @param name			Panel type name (Check viewport_panel_names.h to see a list of some panel names).
 * @param Kv			KeyValues handle with all the data for the panel setup (Depends on the panel type and may be unused).
 * @param show			True to show the panel, or false to remove it from the client screen.
 * @noreturn
 * @error				If the client is not connected an error will be thrown.
 */
native ShowVGUIPanel(client, const String:name[], Handle:Kv=INVALID_HANDLE, bool:show=true);

/**
 * Shows a MOTD panel to a specific client.
 *
 * @param client		Client index.
 * @param title			Title of the panel (printed on the top border of the window).
 * @param msg			Contents of the panel, it can be treated as an url, filename or plain text
 *				  depending on the type parameter (WARNING: msg has to be 192 bytes maximum!)
 * @param type			Determines the way to treat the message body of the panel.
 * @noreturn
 * @error				If the client is not connected an error will be thrown.
 */
stock ShowMOTDPanel(client, const String:title[], const String:msg[], type=MOTDPANEL_TYPE_INDEX)
{
	decl String:num[3];
	new Handle:Kv = CreateKeyValues("data");
	IntToString(type, num, sizeof(num));

	KvSetString(Kv, "title", title);
	KvSetString(Kv, "type", num);
	KvSetString(Kv, "msg", msg);
	ShowVGUIPanel(client, "info", Kv);
	CloseHandle(Kv);
}

/**
 * Displays a panel asking the client to connect to a specified IP.
 *
 * @param client		Client index.
 * @param time			Duration to hold the panel on the client's screen.
 * @param ip			Destionation IP.
 * @noreturn
 */
stock DisplayAskConnectBox(client, Float:time, const String:ip[])
{
	new Handle:Kv = CreateKeyValues("data");
	KvSetFloat(Kv, "time", time);
	KvSetString(Kv, "title", ip);
	CreateDialog(client, Kv, DialogType_AskConnect);
	CloseHandle(Kv);
}
