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

#if defined _sdktools_engine_included
  #endinput
#endif
#define _sdktools_engine_included

#define MAX_LIGHTSTYLES			64

/**
 * Sets a client's "viewing entity."
 *
 * @param client		Client index.
 * @param entity		Entity index.
 * @noreturn
 * @error				Invalid client or entity, lack of mod support, or client not in 
 *						game.
 */
native SetClientViewEntity(client, entity);

/**
 * Sets a light style.
 *
 * @param style			Light style (from 0 to MAX_LIGHTSTYLES-1)
 * @param value			Light value string (see world.cpp/light.cpp in dlls)
 * @noreturn
 * @error				Light style index is out of range.
 */
native SetLightStyle(style, const String:value[]);
