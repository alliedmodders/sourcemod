/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
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

#if defined _sdktools_sound_included
  #endinput
#endif
#define _sdktools_sound_included

/**
 * Sound should be from the target client.
 */
#define SOUND_FROM_PLAYER		-2

/**
 * Sound should be from the listen server player.
 */
#define SOUND_FROM_LOCAL_PLAYER	-1

/**
 * Sound is from the world.
 */
#define SOUND_FROM_WORLD		0

/**
 * Sound channels.
 */
enum
{
	SNDCHAN_REPLACE = -1,		/**< Unknown */
	SNDCHAN_AUTO = 0,			/**< Auto */
	SNDCHAN_WEAPON = 1,			/**< Weapons */
	SNDCHAN_VOICE = 2,			/**< Voices */
	SNDCHAN_ITEM = 3,			/**< Items */
	SNDCHAN_BODY = 4,			/**< Player? */
	SNDCHAN_STREAM = 5,			/**< "Stream channel from the static or dynamic area" */
	SNDCHAN_STATIC = 6,			/**< "Stream channel from the static area" */
	SNDCHAN_VOICE_BASE = 7,		/**< "Channel for network voice data" */
	SNDCHAN_USER_BASE = 135		/**< Anything >= this is allocated to game code */
};

/**
 * Sound flags for the sound emitter system.
 */
enum
{
	SND_NOFLAGS= 0,			/**< Nothing */
	SND_CHANGEVOL = 1,		/**< Change sound volume */
	SND_CHANGEPITCH = 2,	/**< Change sound pitch */
	SND_STOP = 3,			/**< Stop the sound */
	SND_SPAWNING = 4,		/**< Used in some cases for ambients */
	SND_DELAY = 5,			/**< Sound has an initial delay */
	SND_STOPLOOPING = 6,	/**< Stop looping all sounds on the entity */
	SND_SPEAKER = 7,		/**< Being played by a mic through a speaker */
	SND_SHOULDPAUSE = 8,	/**< Pause if game is paused */
};

/**
 * Various predefined sound levels in dB.
 */
enum
{
	SNDLEVEL_NONE = 0,			/**< None */
	SNDLEVEL_RUSTLE = 20,		/**< Rustling leaves */
	SNDLEVEL_WHISPER = 25,		/**< Whispering */
	SNDLEVEL_LIBRARY = 30,		/**< In a library */
	SNDLEVEL_FRIDGE = 45,		/**< Refridgerator */
	SNDLEVEL_HOME = 50,			/**< Average home (3.9 attn) */
	SNDLEVEL_CONVO = 60,		/**< Normal conversation (2.0 attn) */
	SNDLEVEL_DRYER = 60,		/**< Clothes dryer */
	SNDLEVEL_DISHWASHER = 65,	/**< Dishwasher/washing machine (1.5 attn) */
	SNDLEVEL_CAR = 70,			/**< Car or vacuum cleaner (1.0 attn) */
	SNDLEVEL_NORMAL = 75,		/**< Normal sound level */
	SNDLEVEL_TRAFFIC = 75,		/**< Busy traffic (0.8 attn) */
	SNDLEVEL_MINIBIKE = 80,		/**< Mini-bike, alarm clock (0.7 attn) */
	SNDLEVEL_SCREAMING = 90,	/**< Screaming child (0.5 attn) */
	SNDLEVEL_TRAIN = 100,		/**< Subway train, pneumatic drill (0.4 attn) */
	SNDLEVEL_HELICOPTER = 105,	/**< Helicopter */
	SNDLEVEL_SNOWMOBILE = 110,	/**< Snow mobile */
	SNDLEVEL_AIRCRAFT = 120,	/**< Auto horn, aircraft */
	SNDLEVEL_RAIDSIREN = 130,	/**< Air raid siren */
	SNDLEVEL_GUNFIRE = 140,		/**< Gunshot, jet engine (0.27 attn) */
	SNDLEVEL_ROCKET = 180,		/**< Rocket launching (0.2 attn) */
};

#define SNDVOL_NORMAL		1.0		/**< Normal volume */
#define SNDPITCH_NORMAL		100		/**< Normal pitch */
#define SNDPITCH_LOW		95		/**< A low pitch */
#define SNDPITCH_HIGH		120		/**< A high pitch */
#define SNDATTN_NONE		0.0		/**< No attenuation */
#define SNDATTN_NORMAL		0.8		/**< Normal attenuation */
#define SNDATTN_STATIC		1.25	/**< Static attenuation? */
#define SNDATTN_RICOCHET	1.5		/**< Ricochet effect */
#define SNDATTN_IDLE		2.0		/**< Idle attenuation? */
 
/**
 * Prefetches a sound.
 *
 * @param name		Sound file name relative to the "sounds" folder.
 * @noreturn
 */
native PrefetchSound(const String:nane[]);

/**
 * This function is not known to work, and may crash.  You should 
 * not use it.  It is provided for backwards compatibility only.
 *
 * @param name		Sound file name relative to the "sounds" folder.
 * @return			Duration in seconds.
 */
#pragma deprecated Does not work, may crash.
native Float:GetSoundDuration(const String:name[]);

/**
 * Emits an ambient sound.
 *
 * @param name		Sound file name relative to the "sounds" folder.
 * @param pos		Origin of sound.
 * @param entity	Entity index to associate sound with.
 * @param level		Sound level (from 0 to 255).
 * @param flags		Sound flags.
 * @param vol		Volume (from 0.0 to 1.0).
 * @param pitch		Pitch (from 0 to 255).
 * @param delay		Play delay.
 * @noreturn
 */
native EmitAmbientSound(const String:name[],
						const Float:pos[3],
						entity = SOUND_FROM_WORLD,
						level = SNDLEVEL_NORMAL,
						flags = SND_NOFLAGS,
						Float:vol = SNDVOL_NORMAL,
						pitch = SNDPITCH_NORMAL,
						Float:delay = 0.0);

/**
 * Fades a client's volume level toward silence or a given percentage.
 *
 * @param client	Client index.
 * @param percent	Fade percentage.
 * @param outtime	Fade out time, in seconds.
 * @param holdtime	Hold time, in seconds.
 * @param intime	Fade in time, in seconds.
 * @noreturn
 * @error			Invalid client index or client not in game.
 */
native FadeClientVolume(client, Float:percent, Float:outtime, Float:holdtime, Float:intime);

/**
 * Stops a sound.
 *
 * @param entity	Entity index.
 * @param channel	Channel number.
 * @param name		Sound file name relative to the "sounds" folder.
 * @noreturn
 */
native StopSound(entity, channel, const String:name[]);

/**
 * Emits a sound to a list of clients.
 *
 * @param clients		Array of client indexes.
 * @param numClients	Number of clients in the array.
 * @param sample		Sound file name relative to the "sounds" folder.
 * @param entity		Entity to emit from.
 * @param channel		Channel to emit with.
 * @param level			Sound level.
 * @param flags			Sound flags.
 * @param volume		Sound volume.
 * @param pitch			Sound pitch.
 * @param speakerentity	Unknown.
 * @param origin		Sound origin.
 * @param dir			Sound direction.
 * @param updatePos		Unknown (updates positions?)
 * @param soundtime		Alternate time to play sound for.
 * @param ...			Optional list of Float[3] arrays to specify additional origins.
 * @noreturn
 * @error				Invalid client index.
 */
native EmitSound(const clients[],
				 numClients,
				 const String:sample[],
				 entity = SOUND_FROM_PLAYER,
				 channel = SNDCHAN_AUTO,
				 level = SNDLEVEL_NORMAL,
				 flags = SND_NOFLAGS,
				 Float:volume = SNDVOL_NORMAL,
				 pitch = SNDPITCH_NORMAL,
				 speakerentity = -1,
				 const Float:origin[3] = NULL_VECTOR,
				 const Float:dir[3] = NULL_VECTOR,
				 bool:updatePos = true,
				 Float:soundtime = 0.0,
				 any:...);

/**
 * Emits a sentence to a list of clients.
 *
 * @param clients		Array of client indexes.
 * @param numClients	Number of clients in the array.
 * @param sentence		Sentence index (from PrecacheSenteFile).
 * @param entity		Entity to emit from.
 * @param channel		Channel to emit with.
 * @param level			Sound level.
 * @param flags			Sound flags.
 * @param volume		Sound volume.
 * @param pitch			Sound pitch.
 * @param speakerentity	Unknown.
 * @param origin		Sound origin.
 * @param dir			Sound direction.
 * @param updatePos		Unknown (updates positions?)
 * @param soundtime		Alternate time to play sound for.
 * @param ...			Optional list of Float[3] arrays to specify additional origins.
 * @noreturn
 * @error				Invalid client index.
 */
native EmitSentence(const clients[],
				 numClients,
				 sentence,
				 entity,
				 channel = SNDCHAN_AUTO,
				 level = SNDLEVEL_NORMAL,
				 flags = SND_NOFLAGS,
				 Float:volume = SNDVOL_NORMAL,
				 pitch = SNDPITCH_NORMAL,
				 speakerentity = -1,
				 const Float:origin[3] = NULL_VECTOR,
				 const Float:dir[3] = NULL_VECTOR,
				 bool:updatePos = true,
				 Float:soundtime = 0.0,
				 any:...);

/**
 * Called when an ambient sound is about to be emitted to one or more clients.
 *
 * NOTICE: all parameters can be overwritten to modify the default behavior.
 *
 * @param sample	Sound file name relative to the "sounds" folder.
 * @param entity	Entity index associated to the sound.
 * @param volume	Volume (from 0.0 to 1.0).
 * @param level		Sound level (from 0 to 255).
 * @param pitch		Pitch (from 0 to 255).
 * @param pos		Origin of sound.
 * @param flags		Sound flags.
 * @param delay		Play delay.
 * @return		Plugin_Continue to allow the sound to be played, Plugin_Stop to block it, 
 *			  Plugin_Changed when any parameter has been modified.
 */
functag public Action:AmbientSHook(String:sample[PLATFORM_MAX_PATH], &entity, &Float:volume, &level, &pitch, Float:pos[3], &flags, &Float:delay);

/**
 * Called when a sound is going to be emitted to one or more clients.
 * NOTICE: all params can be overwritten to modify the default behaviour.
 *
 * @param clients		Array of client indexes.
 * @param numClients		Number of clients in the array (modify this value if you add/remove elements from the client array).
 * @param sample		Sound file name relative to the "sounds" folder.
 * @param entity		Entity emitting the sound.
 * @param channel		Channel emitting the sound.
 * @param volume		Sound volume.
 * @param level			Sound level.
 * @param pitch			Sound pitch.
 * @param flags			Sound flags.
 * @return			Plugin_Continue to allow the sound to be played, Plugin_Stop to block it, 
 *				  Plugin_Changed when any parameter has been modified.
 */
functag public Action:NormalSHook(clients[64], &numClients, String:sample[PLATFORM_MAX_PATH], &entity, &channel, &Float:volume, &level, &pitch, &flags);

/**
 * Hooks all played ambient sounds.
 *
 * @param hook		Function to use as a hook.
 * @noreturn
 * @error		Invalid function hook.
 */
native AddAmbientSoundHook(AmbientSHook:hook);

/**
 * Hooks all played normal sounds.
 *
 * @param hook		Function to use as a hook.
 * @noreturn
 * @error		Invalid function hook.
 */
native AddNormalSoundHook(NormalSHook:hook);

/**
 * Unhooks all played ambient sounds.
 *
 * @param hook		Function used for the hook.
 * @noreturn
 * @error		Invalid function hook.
 */
native RemoveAmbientSoundHook(AmbientSHook:hook);

/**
 * Unhooks all played normal sounds.
 *
 * @param hook		Function used for the hook.
 * @noreturn
 * @error		Invalid function hook.
 */
native RemoveNormalSoundHook(NormalSHook:hook);
				 
/**
 * Wrapper to emit sound to one client.
 *
 * @param client		Client index.
 * @param sample		Sound file name relative to the "sounds" folder.
 * @param entity		Entity to emit from.
 * @param channel		Channel to emit with.
 * @param level			Sound level.
 * @param flags			Sound flags.
 * @param volume		Sound volume.
 * @param pitch			Sound pitch.
 * @param speakerentity	Unknown.
 * @param origin		Sound origin.
 * @param dir			Sound direction.
 * @param updatePos		Unknown (updates positions?)
 * @param soundtime		Alternate time to play sound for.
 * @noreturn
 * @error				Invalid client index.
 */
stock EmitSoundToClient(client,
				 const String:sample[],
				 entity = SOUND_FROM_PLAYER,
				 channel = SNDCHAN_AUTO,
				 level = SNDLEVEL_NORMAL,
				 flags = SND_NOFLAGS,
				 Float:volume = SNDVOL_NORMAL,
				 pitch = SNDPITCH_NORMAL,
				 speakerentity = -1,
				 const Float:origin[3] = NULL_VECTOR,
				 const Float:dir[3] = NULL_VECTOR,
				 bool:updatePos = true,
				 Float:soundtime = 0.0)
{
	new clients[1];
	clients[0] = client;
	/* Save some work for SDKTools and remove SOUND_FROM_PLAYER references */
	entity = (entity == SOUND_FROM_PLAYER) ? client : entity;
	EmitSound(clients, 1, sample, entity, channel, 
		level, flags, volume, pitch, speakerentity,
		origin, dir, updatePos, soundtime);
}

/**
 * Wrapper to emit sound to all clients.
 *
 * @param sample		Sound file name relative to the "sounds" folder.
 * @param entity		Entity to emit from.
 * @param channel		Channel to emit with.
 * @param level			Sound level.
 * @param flags			Sound flags.
 * @param volume		Sound volume.
 * @param pitch			Sound pitch.
 * @param speakerentity	Unknown.
 * @param origin		Sound origin.
 * @param dir			Sound direction.
 * @param updatePos		Unknown (updates positions?)
 * @param soundtime		Alternate time to play sound for.
 * @noreturn
 * @error				Invalid client index.
 */
stock EmitSoundToAll(const String:sample[],
				 entity = SOUND_FROM_PLAYER,
				 channel = SNDCHAN_AUTO,
				 level = SNDLEVEL_NORMAL,
				 flags = SND_NOFLAGS,
				 Float:volume = SNDVOL_NORMAL,
				 pitch = SNDPITCH_NORMAL,
				 speakerentity = -1,
				 const Float:origin[3] = NULL_VECTOR,
				 const Float:dir[3] = NULL_VECTOR,
				 bool:updatePos = true,
				 Float:soundtime = 0.0)
{
	new maxClients = GetMaxClients();
	new clients[maxClients];
	new total = 0;
	
	for (new i=1; i<=maxClients; i++)
	{
		if (IsClientInGame(i))
		{
			clients[total++] = i;
		}
	}
	
	if (!total)
	{
		return;
	}
	
	EmitSound(clients, total, sample, entity, channel, 
		level, flags, volume, pitch, speakerentity,
		origin, dir, updatePos, soundtime);
}

/**
 * Converts an attenuation value to a sound level.  
 * This function is from the HL2SDK.
 *
 * @param attn		Attenuation value.
 * @return			Integer sound level.
 */
stock ATTN_TO_SNDLEVEL(Float:attn)
{
	if (attn > 0.0)
	{
		return RoundFloat(50.0 + (20.0 / attn));
	}
	return 0;
}
