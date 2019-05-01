/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Basecommands
 * Part of Basecommands plugin, menu and other functionality.
 *
 * SourceMod (C)2004-2011 AlliedModders LLC.  All rights reserved.
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
 
public int Native_IsConVarProtected(Handle hPlugin, int numParams)
{
	int len;
	int error = GetNativeStringLength(1, len);
	
	if (error != SP_ERROR_NONE)
	{
		ThrowNativeError(error, "Could not get length of string");
		return false;
	}
	char[] cvarname = new char[len];
	GetNativeString(1, cvarname, len);
	
	if (FindConVar(cvarname) == INVALID_HANDLE)
	{
		ThrowNativeError(SP_ERROR_NATIVE, "Cvar \"%s\" does not exist.", cvarname);
		return false;
	}	
	return IsVarProtected(cvarname);
}

public int Native_ProtectConVar(Handle hPlugin, int numParams)
{
	int len;
	int error = GetNativeStringLength(1, len);
	
	if (error != SP_ERROR_NONE)
	{
		ThrowNativeError(error, "Could not get length of string");
		return;
	}
	char[] cvarname = new char[len];
	GetNativeString(1, cvarname, len);
	
	if (FindConVar(cvarname) == INVALID_HANDLE)
	{
		ThrowNativeError(SP_ERROR_NATIVE, "Cvar \"%s\" does not exist.", cvarname);
		return;
	}	
	ProtectVar(cvarname);
}
