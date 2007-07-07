/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#if defined _sdktools_sound_included
  #endinput
#endif
#define _sdktools_sound_included

/**
 * Sound is from the client.
 */
#define SOUND_FROM_PLAYER		-1

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
 * @param name		Sound file name.
 * @noreturn
 */
native PrefetchSound(const String:nane[]);

/**
 * Gets a sound's duration (does not work for mp3s).
 *
 * @param name		Sound file name.
 * @return			Duration in seconds.
 */
native Float:GetSoundDuration(const String:name[]);

/**
 * Emits an ambient sound.
 *
 * @param entity	Entity index to associate sound with.
 * @param pos		Origin of sound.
 * @param name		Sound file name.
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
 * @param name		Sound name.
 * @noreturn
 */
native StopSound(entity, channel, const String:name[]);

/**
 * Emits a sound to a list of clients.
 *
 * @param clients		Array of client indexes.
 * @param numClients	Number of clients in the array.
 * @param entity		Entity to emit from.
 * @param channel		Channel to emit with.
 * @param sample		Sound name.
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
				 entity = SOUND_FROM_WORLD,
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
 * @param entity		Entity to emit from.
 * @param channel		Channel to emit with.
 * @param sentence		Sentence index (from PrecacheSenteFile).
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
				 entity = SOUND_FROM_WORLD,
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
 * Wrapper to emit sound to one client.
 *
 * @param client		Client index.
 * @param entity		Entity to emit from.
 * @param channel		Channel to emit with.
 * @param sample		Sound name.
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
 * @param entity		Entity to emit from.
 * @param channel		Channel to emit with.
 * @param sample		Sound name.
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
