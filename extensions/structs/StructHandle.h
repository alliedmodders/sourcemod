/**
* vim: set ts=4 :
* =============================================================================
* SourceMod Sample Extension
* Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_STRUCT_HANDLE_H_
#define _INCLUDE_SOURCEMOD_STRUCT_HANDLE_H_

#include "Struct.h"
#include "vector.h"
#include "basehandle.h"

class StructHandle
{
public:
	StructInfo *info;
	void *data;
	bool canDelete;

	bool GetInt(const char *member, int *value);
	bool GetFloat(const char *member, float *value);
	bool GetVector(const char *member, Vector *vec);
	bool GetEHandle(const char *member, CBaseHandle **handle);
	bool GetString(const char *member, char **string);
	//bool GetStruct(const char *member, handle_t *value);

	bool SetInt(const char *member, int value);
	bool SetFloat(const char *member, float value);
	bool SetVector(const char *member, Vector vec);
	bool SetEHandle(const char *member, CBaseHandle *handle);
	bool SetString(const char *member, char *string);
	//bool SetStruct(const char *member, StructHandle *struct);

	bool IsPointerNull(const char *member, bool *result);
	bool setPointerNull(const char *member);
};

#endif //_INCLUDE_SOURCEMOD_STRUCT_HANDLE_H_
