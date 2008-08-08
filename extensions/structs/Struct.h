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

#ifndef _INCLUDE_SOURCEMOD_STRUCT_H_
#define _INCLUDE_SOURCEMOD_STRUCT_H_

#include "sm_trie_tpl.h"
#include "sh_list.h"

enum MemberType
{
	Member_Unknown,
	Member_Int8,
	Member_Int8Pointer,
	Member_Int16,
	Member_Int16Pointer,
	Member_Int32,
	Member_Int32Pointer,
	Member_Float,
	Member_FloatPointer,
	//Member_Struct,
	//Member_StructPointer,
	Member_Char,
	Member_CharPointer,
	Member_Vector,
	Member_VectorPointer,
	Member_EHandle,
	Member_EHandlePointer,
};

struct MemberInfo
{
	MemberInfo();

	char name[100];
	MemberType type;
	int size;
	int offset;
};

struct StructInfo
{
	StructInfo();
	~StructInfo();
	void AddMember(const char *name, MemberInfo *member);

	char name[100];
	int size;
	KTrie<MemberInfo *> members;
	SourceHook::List<MemberInfo *> membersList;

	bool getOffset(const char *member, int *offset);
	bool getType(const char *member, MemberType*type);
	bool getSize(const char *member, int *size);
};

#endif //_INCLUDE_SOURCEMOD_STRUCT_H_
