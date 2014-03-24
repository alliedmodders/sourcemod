/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
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

#ifndef _INCLUDE_VTABLE_HOOK_HELPER_H_
#define _INCLUDE_VTABLE_HOOK_HELPER_H_

class CVTableHook
{
public:
	CVTableHook(void *takenclass)
	{
		this->vtableptr = *reinterpret_cast<void ***>(takenclass);
		this->hookid = 0;
	}

	CVTableHook(void *vtable, int hook)
	{
		this->vtableptr = vtable;
		this->hookid = hook;
	}

	CVTableHook(CVTableHook &other)
	{
		this->vtableptr = other.vtableptr;
		this->hookid = other.hookid;

		other.hookid = 0;
	}

	CVTableHook(CVTableHook *other)
	{
		this->vtableptr = other->vtableptr;
		this->hookid = other->hookid;

		other->hookid = 0;
	}

	~CVTableHook()
	{
		if (this->hookid)
		{
			SH_REMOVE_HOOK_ID(this->hookid);
			this->hookid = 0;
		}
	}
public:
	void *GetVTablePtr(void)
	{
		return this->vtableptr;
	}

	void SetHookID(int hook)
	{
		this->hookid = hook;
	}

	bool IsHooked(void)
	{
		return (this->hookid != 0);
	}
public:
	bool operator == (CVTableHook &other)
	{
		return (this->GetVTablePtr() == other.GetVTablePtr());
	}

	bool operator == (CVTableHook *other)
	{
		return (this->GetVTablePtr() == other->GetVTablePtr());
	}

	bool operator != (CVTableHook &other)
	{
		return (this->GetVTablePtr() != other.GetVTablePtr());
	}

	bool operator != (CVTableHook *other)
	{
		return (this->GetVTablePtr() != other->GetVTablePtr());
	}
private:
	void *vtableptr;
	int hookid;
};

#endif //_INCLUDE_VTABLE_HOOK_HELPER_H_
