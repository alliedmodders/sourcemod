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

#include "Struct.h"

bool StructInfo::getOffset( const char *member, int *offset )
{
	MemberInfo **pInfo = members.retrieve(member);

	if (pInfo == NULL)
	{
		return false;
	}

	*offset = (*pInfo)->offset;

	return true;
}

bool StructInfo::getType( const char *member, MemberType*type )
{
	MemberInfo **pInfo = members.retrieve(member);

	if (pInfo == NULL)
	{
		return false;
	}

	*type = (*pInfo)->type;

	return true;
}

bool StructInfo::getSize( const char *member, int *size )
{
	MemberInfo **pInfo = members.retrieve(member);

	if (pInfo == NULL)
	{
		return false;
	}

	*size = (*pInfo)->size;

	return true;
}

void StructInfo::AddMember( const char *name, MemberInfo *member )
{
	members.insert(name, member);
	membersList.push_back(member);
}

StructInfo::~StructInfo()
{
	SourceHook::List<MemberInfo *>::iterator iter;

	iter = membersList.begin();

	while (iter != membersList.end())
	{
		delete (MemberInfo *)*iter;
		iter = membersList.erase(iter);
	}

	members.clear();
}

StructInfo::StructInfo()
{
	name[0] = 0;
	size = -1;
}
MemberInfo::MemberInfo()
{
	name[0] = 0;
	size = -1;
	type = Member_Unknown;
	offset = -1;
}
