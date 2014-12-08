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
TopMenu hTopMenu;

// Sounds
new String:g_BlipSound[PLATFORM_MAX_PATH];
new String:g_BeepSound[PLATFORM_MAX_PATH];
new String:g_FinalSound[PLATFORM_MAX_PATH];
new String:g_BoomSound[PLATFORM_MAX_PATH];
new String:g_FreezeSound[PLATFORM_MAX_PATH];

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

new EngineVersion:g_GameEngine = Engine_Unknown;

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
	g_GameEngine = GetEngineVersion();
	g_FadeUserMsgId = GetUserMessageId("Fade");

	RegisterCvars( );
	RegisterCmds( );
	HookEvents( );
	
	/* Account for late loading */
	TopMenu topmenu;
	if (LibraryExists("adminmenu") && ((topmenu = GetAdminTopMenu()) != null))
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
	new Handle:gameConfig = LoadGameConfigFile("funcommands.games");
	if (gameConfig == INVALID_HANDLE)
	{
		SetFailState("Unable to load game config funcommands.games");
		return;
	}
	
	if (GameConfGetKeyValue(gameConfig, "SoundBlip", g_BlipSound, sizeof(g_BlipSound)) && g_BlipSound[0])
	{
		PrecacheSound(g_BlipSound, true);
	}
	
	if (GameConfGetKeyValue(gameConfig, "SoundBeep", g_BeepSound, sizeof(g_BeepSound)) && g_BeepSound[0])
	{
		PrecacheSound(g_BeepSound, true);
	}
	
	if (GameConfGetKeyValue(gameConfig, "SoundFinal", g_FinalSound, sizeof(g_FinalSound)) && g_FinalSound[0])
	{
		PrecacheSound(g_FinalSound, true);
	}
	
	if (GameConfGetKeyValue(gameConfig, "SoundBoom", g_BoomSound, sizeof(g_BoomSound)) && g_BoomSound[0])
	{
		PrecacheSound(g_BoomSound, true);
	}
	
	if (GameConfGetKeyValue(gameConfig, "SoundFreeze", g_FreezeSound, sizeof(g_FreezeSound)) && g_FreezeSound[0])
	{
		PrecacheSound(g_FreezeSound, true);
	}
	
	new String:buffer[PLATFORM_MAX_PATH];
	if (GameConfGetKeyValue(gameConfig, "SpriteBeam", buffer, sizeof(buffer)) && buffer[0])
	{
		g_BeamSprite = PrecacheModel(buffer);
	}
	
	if (GameConfGetKeyValue(gameConfig, "SpriteBeam2", buffer, sizeof(buffer)) && buffer[0])
	{
		g_BeamSprite2 = PrecacheModel(buffer);
	}
	
	if (GameConfGetKeyValue(gameConfig, "SpriteExplosion", buffer, sizeof(buffer)) && buffer[0])
	{
		g_ExplosionSprite = PrecacheModel(buffer);
	}
	
	if (GameConfGetKeyValue(gameConfig, "SpriteGlow", buffer, sizeof(buffer)) && buffer[0])
	{
		g_GlowSprite = PrecacheModel(buffer);
	}
	
	if (GameConfGetKeyValue(gameConfig, "SpriteHalo", buffer, sizeof(buffer)) && buffer[0])
	{
		g_HaloSprite = PrecacheModel(buffer);
	}
	
	CloseHandle(gameConfig);
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

public OnAdminMenuReady(TopMenu topmenu)
{
	/* Block us from being called twice */
	if (topmenu == hTopMenu)
	{
		return;
	}
	
	/* Save the Handle */
	hTopMenu = topmenu;
	
	/* Find the "Player Commands" category */
	TopMenuObject player_commands = hTopMenu.FindCategory(ADMINMENU_PLAYERCOMMANDS);

	if (player_commands != INVALID_TOPMENUOBJECT)
	{
		hTopMenu.AddItem("sm_beacon", AdminMenu_Beacon, player_commands, "sm_beacon", ADMFLAG_SLAY);
		hTopMenu.AddItem("sm_timebomb", AdminMenu_TimeBomb, player_commands, "sm_timebomb", ADMFLAG_SLAY);
		hTopMenu.AddItem("sm_burn", AdminMenu_Burn, player_commands, "sm_burn", ADMFLAG_SLAY);
		hTopMenu.AddItem("sm_firebomb", AdminMenu_FireBomb, player_commands, "sm_firebomb", ADMFLAG_SLAY);
		hTopMenu.AddItem("sm_freeze", AdminMenu_Freeze, player_commands, "sm_freeze", ADMFLAG_SLAY);
		hTopMenu.AddItem("sm_freezebomb", AdminMenu_FreezeBomb, player_commands, "sm_freezebomb", ADMFLAG_SLAY);
		hTopMenu.AddItem("sm_gravity", AdminMenu_Gravity, player_commands, "sm_gravity", ADMFLAG_SLAY);
		hTopMenu.AddItem("sm_blind", AdminMenu_Blind, player_commands, "sm_blind", ADMFLAG_SLAY);
		hTopMenu.AddItem("sm_noclip", AdminMenu_NoClip, player_commands, "sm_noclip", ADMFLAG_SLAY);
		hTopMenu.AddItem("sm_drug", AdminMenu_Drug, player_commands, "sm_drug", ADMFLAG_SLAY);
	}
}

AddTranslatedMenuItem(Handle:menu, const String:opt[], const String:phrase[], client)
{
	decl String:buffer[128];
	Format(buffer, sizeof(buffer), "%T", phrase, client);
	AddMenuItem(menu, opt, buffer);
}

