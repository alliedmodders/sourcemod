/**
 * vim: set ts=4 sw=4 tw=99 noet:
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

#if defined _core_included
 #endinput
#endif
#define _core_included

#include <version>

/** If this gets changed, you need to update Core's check. */
#define SOURCEMOD_PLUGINAPI_VERSION     5

struct PlVers
{
	public int version;
	public const char[] filevers;
	public const char[] date;
	public const char[] time;
};

/**
 * Specifies what to do after a hook completes.
 */
enum Action
{
	Plugin_Continue = 0,    /**< Continue with the original action */
	Plugin_Changed = 1,     /**< Inputs or outputs have been overridden with new values */
	Plugin_Handled = 3,     /**< Handle the action at the end (don't call it) */
	Plugin_Stop = 4         /**< Immediately stop the hook chain and handle the original */
};

/**
 * Specifies identity types.
 */
enum Identity
{
	Identity_Core = 0,
	Identity_Extension = 1,
	Identity_Plugin = 2
};

public PlVers __version = 
{
	version = SOURCEMOD_PLUGINAPI_VERSION,
	filevers = SOURCEMOD_VERSION,
	date = __DATE__,
	time = __TIME__
};

/**
 * Plugin status values.
 */
enum PluginStatus
{
	Plugin_Running=0,       /**< Plugin is running */
	/* All states below are "temporarily" unexecutable */
	Plugin_Paused,          /**< Plugin is loaded but paused */
	Plugin_Error,           /**< Plugin is loaded but errored/locked */
	/* All states below do not have all natives */
	Plugin_Loaded,          /**< Plugin has passed loading and can be finalized */
	Plugin_Failed,          /**< Plugin has a fatal failure */
	Plugin_Created,         /**< Plugin is created but not initialized */
	Plugin_Uncompiled,      /**< Plugin is not yet compiled by the JIT */
	Plugin_BadLoad,         /**< Plugin failed to load */
	Plugin_Evicted          /**< Plugin was unloaded due to an error */
};

/**
 * Plugin information properties. Plugins can declare a global variable with
 * their info. Example,
 *
 *   public Plugin myinfo = {
 *   	name = "Admin Help",
 *   	author = "AlliedModders LLC",
 *   	description = "Display command information",
 *   	version = "1.0",
 *   	url = "http://www.sourcemod.net/"
 *   };
 *
 * SourceMod will display this information when a user inspects plugins in the
 * console.
 */
enum PluginInfo
{
	PlInfo_Name,            /**< Plugin name */
	PlInfo_Author,          /**< Plugin author */
	PlInfo_Description,     /**< Plugin description */
	PlInfo_Version,         /**< Plugin version */
	PlInfo_URL              /**< Plugin URL */
};

/**
 * Defines how an extension must expose itself for autoloading.
 */
struct Extension
{
	public const char[] name;   /**< Short name */
	public const char[] file;   /**< Default file name */
	public bool autoload;       /**< Whether or not to auto-load */
	public bool required;       /**< Whether or not to require */
};

/**
 * Defines how a plugin must expose itself for native requiring.
 */
struct SharedPlugin
{
	public const char[] name;   /**< Short name */
	public const char[] file;   /**< File name */
	public bool required;       /**< Whether or not to require */
};

public float NULL_VECTOR[3];        /**< Pass this into certain functions to act as a C++ NULL */
public const char NULL_STRING[1];   /**< pass this into certain functions to act as a C++ NULL */

/**
 * Check if the given vector is the NULL_VECTOR.
 *
 * @param vec     The vector to test.
 * @return        True if NULL_VECTOR, false otherwise.
 */
native bool IsNullVector(const float vec[3]);

/**
 * Check if the given string is the NULL_STRING.
 *
 * @param str     The string to test.
 * @return        True if NULL_STRING, false otherwise.
 */
native bool IsNullString(const char[] str);

/**
 * Horrible compatibility shim.
 */
public Extension __ext_core = 
{
	name = "Core",
	file = "core",
	autoload = 0,
	required = 0,
};

native int VerifyCoreVersion();

/**
 * Sets a native as optional, such that if it is unloaded, removed,
 * or otherwise non-existent, the plugin will still work.  Calling
 * removed natives results in a run-time error.
 *
 * @param name          Native name.
 */
native void MarkNativeAsOptional(const char[] name);

public void __ext_core_SetNTVOptional()
{
	MarkNativeAsOptional("GetFeatureStatus");
	MarkNativeAsOptional("RequireFeature");
	MarkNativeAsOptional("AddCommandListener");
	MarkNativeAsOptional("RemoveCommandListener");

	MarkNativeAsOptional("BfWriteBool");
	MarkNativeAsOptional("BfWriteByte");
	MarkNativeAsOptional("BfWriteChar");
	MarkNativeAsOptional("BfWriteShort");
	MarkNativeAsOptional("BfWriteWord");
	MarkNativeAsOptional("BfWriteNum");
	MarkNativeAsOptional("BfWriteFloat");
	MarkNativeAsOptional("BfWriteString");
	MarkNativeAsOptional("BfWriteEntity");
	MarkNativeAsOptional("BfWriteAngle");
	MarkNativeAsOptional("BfWriteCoord");
	MarkNativeAsOptional("BfWriteVecCoord");
	MarkNativeAsOptional("BfWriteVecNormal");
	MarkNativeAsOptional("BfWriteAngles");
	MarkNativeAsOptional("BfReadBool");
	MarkNativeAsOptional("BfReadByte");
	MarkNativeAsOptional("BfReadChar");
	MarkNativeAsOptional("BfReadShort");
	MarkNativeAsOptional("BfReadWord");
	MarkNativeAsOptional("BfReadNum");
	MarkNativeAsOptional("BfReadFloat");
	MarkNativeAsOptional("BfReadString");
	MarkNativeAsOptional("BfReadEntity");
	MarkNativeAsOptional("BfReadAngle");
	MarkNativeAsOptional("BfReadCoord");
	MarkNativeAsOptional("BfReadVecCoord");
	MarkNativeAsOptional("BfReadVecNormal");
	MarkNativeAsOptional("BfReadAngles");
	MarkNativeAsOptional("BfGetNumBytesLeft");

	MarkNativeAsOptional("BfWrite.WriteBool");
	MarkNativeAsOptional("BfWrite.WriteByte");
	MarkNativeAsOptional("BfWrite.WriteChar");
	MarkNativeAsOptional("BfWrite.WriteShort");
	MarkNativeAsOptional("BfWrite.WriteWord");
	MarkNativeAsOptional("BfWrite.WriteNum");
	MarkNativeAsOptional("BfWrite.WriteFloat");
	MarkNativeAsOptional("BfWrite.WriteString");
	MarkNativeAsOptional("BfWrite.WriteEntity");
	MarkNativeAsOptional("BfWrite.WriteAngle");
	MarkNativeAsOptional("BfWrite.WriteCoord");
	MarkNativeAsOptional("BfWrite.WriteVecCoord");
	MarkNativeAsOptional("BfWrite.WriteVecNormal");
	MarkNativeAsOptional("BfWrite.WriteAngles");
	MarkNativeAsOptional("BfRead.ReadBool");
	MarkNativeAsOptional("BfRead.ReadByte");
	MarkNativeAsOptional("BfRead.ReadChar");
	MarkNativeAsOptional("BfRead.ReadShort");
	MarkNativeAsOptional("BfRead.ReadWord");
	MarkNativeAsOptional("BfRead.ReadNum");
	MarkNativeAsOptional("BfRead.ReadFloat");
	MarkNativeAsOptional("BfRead.ReadString");
	MarkNativeAsOptional("BfRead.ReadEntity");
	MarkNativeAsOptional("BfRead.ReadAngle");
	MarkNativeAsOptional("BfRead.ReadCoord");
	MarkNativeAsOptional("BfRead.ReadVecCoord");
	MarkNativeAsOptional("BfRead.ReadVecNormal");
	MarkNativeAsOptional("BfRead.ReadAngles");
	MarkNativeAsOptional("BfRead.BytesLeft.get");

	MarkNativeAsOptional("PbReadInt");
	MarkNativeAsOptional("PbReadFloat");
	MarkNativeAsOptional("PbReadBool");
	MarkNativeAsOptional("PbReadString");
	MarkNativeAsOptional("PbReadColor");
	MarkNativeAsOptional("PbReadAngle");
	MarkNativeAsOptional("PbReadVector");
	MarkNativeAsOptional("PbReadVector2D");
	MarkNativeAsOptional("PbGetRepeatedFieldCount");
	MarkNativeAsOptional("PbSetInt");
	MarkNativeAsOptional("PbSetFloat");
	MarkNativeAsOptional("PbSetBool");
	MarkNativeAsOptional("PbSetString");
	MarkNativeAsOptional("PbSetColor");
	MarkNativeAsOptional("PbSetAngle");
	MarkNativeAsOptional("PbSetVector");
	MarkNativeAsOptional("PbSetVector2D");
	MarkNativeAsOptional("PbAddInt");
	MarkNativeAsOptional("PbAddFloat");
	MarkNativeAsOptional("PbAddBool");
	MarkNativeAsOptional("PbAddString");
	MarkNativeAsOptional("PbAddColor");
	MarkNativeAsOptional("PbAddAngle");
	MarkNativeAsOptional("PbAddVector");
	MarkNativeAsOptional("PbAddVector2D");
	MarkNativeAsOptional("PbRemoveRepeatedFieldValue");
	MarkNativeAsOptional("PbReadMessage");
	MarkNativeAsOptional("PbReadRepeatedMessage");
	MarkNativeAsOptional("PbAddMessage");

	MarkNativeAsOptional("Protobuf.ReadInt");
	MarkNativeAsOptional("Protobuf.ReadInt64");
	MarkNativeAsOptional("Protobuf.ReadFloat");
	MarkNativeAsOptional("Protobuf.ReadBool");
	MarkNativeAsOptional("Protobuf.ReadString");
	MarkNativeAsOptional("Protobuf.ReadColor");
	MarkNativeAsOptional("Protobuf.ReadAngle");
	MarkNativeAsOptional("Protobuf.ReadVector");
	MarkNativeAsOptional("Protobuf.ReadVector2D");
	MarkNativeAsOptional("Protobuf.GetRepeatedFieldCount");
	MarkNativeAsOptional("Protobuf.SetInt");
	MarkNativeAsOptional("Protobuf.SetInt64");
	MarkNativeAsOptional("Protobuf.SetFloat");
	MarkNativeAsOptional("Protobuf.SetBool");
	MarkNativeAsOptional("Protobuf.SetString");
	MarkNativeAsOptional("Protobuf.SetColor");
	MarkNativeAsOptional("Protobuf.SetAngle");
	MarkNativeAsOptional("Protobuf.SetVector");
	MarkNativeAsOptional("Protobuf.SetVector2D");
	MarkNativeAsOptional("Protobuf.AddInt");
	MarkNativeAsOptional("Protobuf.AddInt64");
	MarkNativeAsOptional("Protobuf.AddFloat");
	MarkNativeAsOptional("Protobuf.AddBool");
	MarkNativeAsOptional("Protobuf.AddString");
	MarkNativeAsOptional("Protobuf.AddColor");
	MarkNativeAsOptional("Protobuf.AddAngle");
	MarkNativeAsOptional("Protobuf.AddVector");
	MarkNativeAsOptional("Protobuf.AddVector2D");
	MarkNativeAsOptional("Protobuf.RemoveRepeatedFieldValue");
	MarkNativeAsOptional("Protobuf.ReadMessage");
	MarkNativeAsOptional("Protobuf.ReadRepeatedMessage");
	MarkNativeAsOptional("Protobuf.AddMessage");

	VerifyCoreVersion();
}


#define AUTOLOAD_EXTENSIONS
#define REQUIRE_EXTENSIONS
#define REQUIRE_PLUGIN
