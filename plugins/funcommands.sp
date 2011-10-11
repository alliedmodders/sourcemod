/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basic Fun Commands Plugin
 * Implements basic punishment commands.
 *
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
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

#pragma semicolon 1

#include <sourcemod>
#include <sdktools>
#undef REQUIRE_PLUGIN
#include <adminmenu>

public Plugin:myinfo =
{
	name = "Fun Commands",
	author = "AlliedModders LLC",
	description = "Fun Commands",
	version = SOURCEMOD_VERSION,
	url = "http://www.sourcemod.net/"
};

// Admin Menu
new Handle:hTopMenu = INVALID_HANDLE;

// Sounds
#define SOUND_BLIP		"buttons/blip1.wav"
#define SOUND_BEEP		"buttons/button17.wav"
#define SOUND_FINAL		"weapons/cguard/charging.wav"
#define SOUND_BOOM		"weapons/explode3.wav"
#define SOUND_FREEZE	"physics/glass/glass_impact_bullet4.wav"

// Following are model indexes for temp entities
new g_BeamSprite        = -1;
new g_BeamSprite2       = -1;
new g_HaloSprite        = -1;
new g_GlowSprite        = -1;
new g_ExplosionSprite   = -1;

// Basic color arrays for temp entities
new redColor[4]		= {255, 75, 75, 255};
new orangeColor[4]	= {255, 128, 0, 255};
new greenColor[4]	= {75, 255, 75, 255};
new blueColor[4]	= {75, 75, 255, 255};
new whiteColor[4]	= {255, 255, 255, 255};
new greyColor[4]	= {128, 128, 128, 255};

// UserMessageId for Fade.
new UserMsg:g_FadeUserMsgId;

// Serial Generator for Timer Safety
new g_Serial_Gen = 0;

new g_GameEngine = SOURCE_SDK_UNKNOWN;

// Flags used in various timers
#define DEFAULT_TIMER_FLAGS TIMER_REPEAT|TIMER_FLAG_NO_MAPCHANGE

// Include various commands and supporting functions
#include "funcommands/beacon.sp"
#include "funcommands/timebomb.sp"
#include "funcommands/fire.sp"
#include "funcommands/ice.sp"
#include "funcommands/gravity.sp"
#include "funcommands/blind.sp"
#include "funcommands/noclip.sp"
#include "funcommands/drug.sp"

public OnPluginStart()
{
	if (FindPluginByFile("basefuncommands.smx") != INVALID_HANDLE)
	{
		ThrowError("This plugin replaces basefuncommands.  You cannot run both at once.");
	}
	
	LoadTranslations("common.phrases");
	LoadTranslations("funcommands.phrases");
	g_GameEngine = GuessSDKVersion();
	g_FadeUserMsgId = GetUserMessageId("Fade");

	RegisterCvars( );
	RegisterCmds( );
	HookEvents( );
	
	/* Account for late loading */
	new Handle:topmenu;
	if (LibraryExists("adminmenu") && ((topmenu = GetAdminTopMenu()) != INVALID_HANDLE))
	{
		OnAdminMenuReady(topmenu);
	}
}

RegisterCvars( )
{
	// beacon
	g_Cvar_BeaconRadius = CreateConVar("sm_beacon_radius", "375", "Sets the radius for beacon's light rings.", 0, true, 50.0, true, 1500.0);

	// timebomb
	g_Cvar_TimeBombTicks = CreateConVar("sm_timebomb_ticks", "10.0", "Sets how long the timebomb fuse is.", 0, true, 5.0, true, 120.0);
	g_Cvar_TimeBombRadius = CreateConVar("sm_timebomb_radius", "600", "Sets the bomb blast radius.", 0, true, 50.0, true, 3000.0);
	g_Cvar_TimeBombMode = CreateConVar("sm_timebomb_mode", "0", "Who is killed by the timebomb? 0 = Target only, 1 = Target's team, 2 = Everyone", 0, true, 0.0, true, 2.0);
	
	// fire
	g_Cvar_BurnDuration = CreateConVar("sm_burn_duration", "20.0", "Sets the default duration of sm_burn and firebomb victims.", 0, true, 0.5, true, 20.0);
	g_Cvar_FireBombTicks = CreateConVar("sm_firebomb_ticks", "10.0", "Sets how long the FireBomb fuse is.", 0, true, 5.0, true, 120.0);
	g_Cvar_FireBombRadius = CreateConVar("sm_firebomb_radius", "600", "Sets the bomb blast radius.", 0, true, 50.0, true, 3000.0);
	g_Cvar_FireBombMode = CreateConVar("sm_firebomb_mode", "0", "Who is targetted by the FireBomb? 0 = Target only, 1 = Target's team, 2 = Everyone", 0, true, 0.0, true, 2.0);
	
	// ice
	g_Cvar_FreezeDuration = CreateConVar("sm_freeze_duration", "10.0", "Sets the default duration for sm_freeze and freezebomb victims", 0, true, 1.0, true, 120.0);	
	g_Cvar_FreezeBombTicks = CreateConVar("sm_freezebomb_ticks", "10.0", "Sets how long the freezebomb fuse is.", 0, true, 5.0, true, 120.0);
	g_Cvar_FreezeBombRadius = CreateConVar("sm_freezebomb_radius", "600", "Sets the freezebomb blast radius.", 0, true, 50.0, true, 3000.0);
	g_Cvar_FreezeBombMode = CreateConVar("sm_freezebomb_mode", "0", "Who is targetted by the freezebomb? 0 = Target only, 1 = Target's team, 2 = Everyone", 0, true, 0.0, true, 2.0);
	
	AutoExecConfig(true, "funcommands");
}

RegisterCmds( )
{
	RegAdminCmd("sm_beacon", Command_Beacon, ADMFLAG_SLAY, "sm_beacon <#userid|name> [0/1]");
	RegAdminCmd("sm_timebomb", Command_TimeBomb, ADMFLAG_SLAY, "sm_timebomb <#userid|name> [0/1]");
	RegAdminCmd("sm_burn", Command_Burn, ADMFLAG_SLAY, "sm_burn <#userid|name> [time]");
	RegAdminCmd("sm_firebomb", Command_FireBomb, ADMFLAG_SLAY, "sm_firebomb <#userid|name> [0/1]");
	RegAdminCmd("sm_freeze", Command_Freeze, ADMFLAG_SLAY, "sm_freeze <#userid|name> [time]");
	RegAdminCmd("sm_freezebomb", Command_FreezeBomb, ADMFLAG_SLAY, "sm_freezebomb <#userid|name> [0/1]");
	RegAdminCmd("sm_gravity", Command_Gravity, ADMFLAG_SLAY, "sm_gravity <#userid|name> [amount] - Leave amount off to reset. Amount is 0.0 through 5.0");
	RegAdminCmd("sm_blind", Command_Blind, ADMFLAG_SLAY, "sm_blind <#userid|name> [amount] - Leave amount off to reset.");
	RegAdminCmd("sm_noclip", Command_NoClip, ADMFLAG_SLAY|ADMFLAG_CHEATS, "sm_noclip <#userid|name>");
	RegAdminCmd("sm_drug", Command_Drug, ADMFLAG_SLAY, "sm_drug <#userid|name> [0/1]");
}

HookEvents( )
{
	decl String:folder[64];
	GetGameFolderName(folder, sizeof(folder));

	if (strcmp(folder, "tf") == 0)
	{
		HookEvent("teamplay_win_panel", Event_RoundEnd, EventHookMode_PostNoCopy);
		HookEvent("teamplay_restart_round", Event_RoundEnd, EventHookMode_PostNoCopy);
		HookEvent("arena_win_panel", Event_RoundEnd, EventHookMode_PostNoCopy);
	}
	else if (strcmp(folder, "nucleardawn") == 0)
	{
		HookEvent("round_win", Event_RoundEnd, EventHookMode_PostNoCopy);
	}
	else
	{
		HookEvent("round_end", Event_RoundEnd, EventHookMode_PostNoCopy);
	}	
}

public OnMapStart()
{
	PrecacheSound(SOUND_BLIP, true);
	PrecacheSound(SOUND_BEEP, true);
	PrecacheSound(SOUND_FINAL, true);
	PrecacheSound(SOUND_BOOM, true);
	PrecacheSound(SOUND_FREEZE, true);

	new sdkversion = GuessSDKVersion();
	if (sdkversion == SOURCE_SDK_LEFT4DEAD || sdkversion == SOURCE_SDK_LEFT4DEAD2)
	{
		g_BeamSprite = PrecacheModel("materials/sprites/laserbeam.vmt");
		g_HaloSprite = PrecacheModel("materials/sprites/glow01.vmt");
	}
	else
	{
		g_BeamSprite = PrecacheModel("materials/sprites/laser.vmt");
		g_HaloSprite = PrecacheModel("materials/sprites/halo01.vmt");
		g_BeamSprite2 = PrecacheModel("sprites/bluelight1.vmt");
		g_GlowSprite = PrecacheModel("sprites/blueglow2.vmt");
		g_ExplosionSprite = PrecacheModel("sprites/sprite_fire01.vmt");
	}
	
}

public OnMapEnd()
{
	KillAllBeacons( );
	KillAllTimeBombs();
	KillAllFireBombs();
	KillAllFreezes();
	KillAllDrugs();
}

public Action:Event_RoundEnd(Handle:event,const String:name[],bool:dontBroadcast)
{
	KillAllBeacons( );
	KillAllTimeBombs();
	KillAllFireBombs();
	KillAllFreezes();
	KillAllDrugs();
}

public OnAdminMenuReady(Handle:topmenu)
{
	/* Block us from being called twice */
	if (topmenu == hTopMenu)
	{
		return;
	}
	
	/* Save the Handle */
	hTopMenu = topmenu;
	
	/* Find the "Player Commands" category */
	new TopMenuObject:player_commands = FindTopMenuCategory(hTopMenu, ADMINMENU_PLAYERCOMMANDS);

	if (player_commands != INVALID_TOPMENUOBJECT)
	{
		AddToTopMenu(hTopMenu,
			"sm_beacon",
			TopMenuObject_Item,
			AdminMenu_Beacon,
			player_commands,
			"sm_beacon",
			ADMFLAG_SLAY);
	
		AddToTopMenu(hTopMenu,
			"sm_timebomb",
			TopMenuObject_Item,
			AdminMenu_TimeBomb,
			player_commands,
			"sm_timebomb",
			ADMFLAG_SLAY);

		AddToTopMenu(hTopMenu,
			"sm_burn",
			TopMenuObject_Item,
			AdminMenu_Burn,
			player_commands,
			"sm_burn",
			ADMFLAG_SLAY);
		
		AddToTopMenu(hTopMenu,
			"sm_firebomb",
			TopMenuObject_Item,
			AdminMenu_FireBomb,
			player_commands,
			"sm_firebomb",
			ADMFLAG_SLAY);

		AddToTopMenu(hTopMenu,
			"sm_freeze",
			TopMenuObject_Item,
			AdminMenu_Freeze,
			player_commands,
			"sm_freeze",
			ADMFLAG_SLAY);
			
		AddToTopMenu(hTopMenu,
			"sm_freezebomb",
			TopMenuObject_Item,
			AdminMenu_FreezeBomb,
			player_commands,
			"sm_freezebomb",
			ADMFLAG_SLAY);

		AddToTopMenu(hTopMenu,
			"sm_gravity",
			TopMenuObject_Item,
			AdminMenu_Gravity,
			player_commands,
			"sm_gravity",
			ADMFLAG_SLAY);

		AddToTopMenu(hTopMenu,
			"sm_blind",
			TopMenuObject_Item,
			AdminMenu_Blind,
			player_commands,
			"sm_blind",
			ADMFLAG_SLAY);

		AddToTopMenu(hTopMenu,
			"sm_noclip",
			TopMenuObject_Item,
			AdminMenu_NoClip,
			player_commands,
			"sm_noclip",
			ADMFLAG_SLAY);

		AddToTopMenu(hTopMenu,
			"sm_drug",
			TopMenuObject_Item,
			AdminMenu_Drug,
			player_commands,
			"sm_drug",
			ADMFLAG_SLAY);
	}
}

